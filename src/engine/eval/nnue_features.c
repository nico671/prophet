#include "nnue_features.h"

#include "chess/core/bitboard.h"

#include <stdlib.h>

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

static int feature_plane(PieceType piece, bool friendly)
{
    if (piece == KING) {
        return 10;
    }
    return (int)piece - (int)PAWN + (friendly ? 0 : 5);
}

static bool append_piece_features(const CBoard* board, Color perspective, Color color,
                                  PieceType piece, bool mirror, uint16_t* output, size_t capacity,
                                  size_t* count, int king_bucket)
{
    const bool friendly = color == perspective;
    const int plane     = feature_plane(piece, friendly);
    Bitboard pieces     = board->piece_bbs[color][piece];

    while (pieces) {
        Square square             = (Square)bitboard_pop_lsb_unsafe(&pieces);
        Square transformed_square = transform_square(square, perspective, mirror);
        int feature               = ((king_bucket * 11 + plane) * 64) + (int)transformed_square;
        if (feature < 0 || feature >= NNUE_FEATURE_COUNT || *count >= capacity) {
            return false;
        }
        output[(*count)++] = (uint16_t)feature;
    }

    return true;
}

static int compare_uint16(const void* lhs, const void* rhs)
{
    const uint16_t left  = *(const uint16_t*)lhs;
    const uint16_t right = *(const uint16_t*)rhs;
    return (left > right) - (left < right);
}

bool nnue_generate_features(const CBoard* board, NnueFeatureSet feature_set, Color perspective,
                            uint16_t* output, size_t capacity, size_t* count)
{
    if (feature_set != NNUE_FEATURE_HALFKAV2_HM_V1 || (perspective != WHITE && perspective != BLACK)
        || !board_has_valid_piece_placement(board) || !output || !count) {
        return false;
    }

    *count = 0;
    const Square friendly_king
        = (Square)bitboard_lsb_index_safe(board->piece_bbs[perspective][KING]);
    const Square transformed_king = transform_square(friendly_king, perspective, false);
    const bool mirror             = (transformed_king % 8) < 4;
    const Square canonical_king   = transform_square(friendly_king, perspective, mirror);
    const int king_bucket         = ((int)canonical_king / 8) * 4 + ((int)canonical_king % 8 - 4);

    if (king_bucket < 0 || king_bucket >= 32) {
        return false;
    }

    const Color enemy = color_opposite(perspective);
    for (PieceType piece = PAWN; piece <= QUEEN; piece++) {
        if (!append_piece_features(board, perspective, perspective, piece, mirror, output, capacity,
                                   count, king_bucket)
            || !append_piece_features(board, perspective, enemy, piece, mirror, output, capacity,
                                      count, king_bucket)) {
            return false;
        }
    }

    if (!append_piece_features(board, perspective, perspective, KING, mirror, output, capacity,
                               count, king_bucket)
        || !append_piece_features(board, perspective, enemy, KING, mirror, output, capacity, count,
                                  king_bucket)) {
        return false;
    }

    qsort(output, *count, sizeof(*output), compare_uint16);
    return true;
}
