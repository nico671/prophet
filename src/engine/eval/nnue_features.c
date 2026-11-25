#include "nnue_features.h"

#include "chess/core/bitboard.h"

#include <stdlib.h>

/**
 * Verifies the feature mapper's structural board preconditions.
 *
 * Search positions are valid. This check protects public feature APIs from
 * partially initialized boards, missing kings, overlapping bitboards, invalid
 * pawn ranks, and positions that exceed the fixed feature buffer.
 */
static bool board_has_valid_piece_placement(const CBoard* board)
{
    if (!board) {
        return false;
    }

    Bitboard occupied = 0ULL;
    for (Color color = WHITE; color <= BLACK; color++) {
        for (PieceType piece = PAWN; piece <= KING; piece++) {
            Bitboard pieces = board->piece_bbs[color][piece];
            if (pieces & occupied) {
                return false;
            }
            occupied |= pieces;
        }
    }

    if (bitboard_popcount(board->piece_bbs[WHITE][KING]) != 1
        || bitboard_popcount(board->piece_bbs[BLACK][KING]) != 1) {
        return false;
    }

    if ((board->piece_bbs[WHITE][PAWN] & (RANK_1 | RANK_8))
        || (board->piece_bbs[BLACK][PAWN] & (RANK_1 | RANK_8))) {
        return false;
    }

    return bitboard_popcount(occupied) <= NNUE_MAX_ACTIVE_FEATURES;
}

/** Transforms one square into a perspective's vertically flipped and mirrored frame. */
static Square transform_square(Square square, Color perspective, bool mirror)
{
    Square transformed = square;
    if (perspective == BLACK) {
        transformed = square_flip_vertical(transformed);
    }
    if (mirror) {
        transformed = (Square)(transformed ^ 7);
    }
    return transformed;
}

/** Returns the HalfKAv2_hm piece plane for one piece and perspective relation. */
static int feature_plane(PieceType piece, bool friendly)
{
    if (piece == KING) {
        return 10;
    }
    return (int)piece - (int)PAWN + (friendly ? 0 : 5);
}

typedef struct {
    bool mirror;
    int king_bucket;
} NnuePerspective;

/** Builds the shared king-bucket and mirror state for one feature perspective. */
static bool perspective_for_board(const CBoard* board, Color perspective, NnuePerspective* output)
{
    if ((perspective != WHITE && perspective != BLACK) || !output
        || !board_has_valid_piece_placement(board)) {
        return false;
    }
    const Square friendly_king
        = (Square)bitboard_lsb_index_safe(board->piece_bbs[perspective][KING]);
    const Square transformed_king = transform_square(friendly_king, perspective, false);
    output->mirror                = (transformed_king % 8) < 4;
    const Square canonical_king   = transform_square(friendly_king, perspective, output->mirror);
    output->king_bucket           = ((int)canonical_king / 8) * 4 + ((int)canonical_king % 8 - 4);
    return output->king_bucket >= 0 && output->king_bucket < 32;
}

bool nnue_feature_index_for_piece(const CBoard* board, Color perspective, Color color,
                                  PieceType piece, Square square, uint16_t* output)
{
    NnuePerspective state;
    if ((color != WHITE && color != BLACK) || piece < PAWN || piece > KING || square >= NO_SQUARE
        || !output || !perspective_for_board(board, perspective, &state)) {
        return false;
    }
    const int feature = ((state.king_bucket * 11 + feature_plane(piece, color == perspective)) * 64)
        + (int)transform_square(square, perspective, state.mirror);
    if (feature < 0 || feature >= NNUE_FEATURE_COUNT) {
        return false;
    }
    *output = (uint16_t)feature;
    return true;
}

/** Appends all feature indices for one color and piece type using prepared perspective state. */
static bool append_piece_features(const CBoard* board, Color perspective, Color color,
                                  PieceType piece, const NnuePerspective* state, uint16_t* output,
                                  size_t capacity, size_t* count)
{
    const bool friendly = color == perspective;
    const int plane     = feature_plane(piece, friendly);
    Bitboard pieces     = board->piece_bbs[color][piece];

    while (pieces) {
        Square square             = (Square)bitboard_pop_lsb_unsafe(&pieces);
        Square transformed_square = transform_square(square, perspective, state->mirror);
        int feature = ((state->king_bucket * 11 + plane) * 64) + (int)transformed_square;
        if (feature < 0 || feature >= NNUE_FEATURE_COUNT || *count >= capacity) {
            return false;
        }
        output[(*count)++] = (uint16_t)feature;
    }

    return true;
}

/** Orders feature indices numerically for the exported feature-list contract. */
static int compare_uint16(const void* lhs, const void* rhs)
{
    const uint16_t left  = *(const uint16_t*)lhs;
    const uint16_t right = *(const uint16_t*)rhs;
    return (left > right) - (left < right);
}

bool nnue_generate_features(const CBoard* board, Color perspective, uint16_t* output,
                            size_t capacity, size_t* count)
{
    NnuePerspective state;
    if (!output || !count || !perspective_for_board(board, perspective, &state)) {
        return false;
    }

    *count            = 0;
    const Color enemy = color_opposite(perspective);
    for (PieceType piece = PAWN; piece <= QUEEN; piece++) {
        if (!append_piece_features(board, perspective, perspective, piece, &state, output, capacity,
                                   count)
            || !append_piece_features(board, perspective, enemy, piece, &state, output, capacity,
                                      count)) {
            return false;
        }
    }

    if (!append_piece_features(board, perspective, perspective, KING, &state, output, capacity,
                               count)
        || !append_piece_features(board, perspective, enemy, KING, &state, output, capacity,
                                  count)) {
        return false;
    }

    qsort(output, *count, sizeof(*output), compare_uint16);
    return true;
}
