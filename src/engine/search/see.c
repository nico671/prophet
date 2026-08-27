#include "engine/search/see.h"

#include "chess/core/bitboard.h"
#include "chess/movegen/constant_attacks.h"
#include "chess/movegen/sliding_attacks.h"
#include "engine/eval/hceval.h"

#include <stdbool.h>

typedef struct {
    Bitboard piece_bbs[2][7];
    Bitboard occupancy_bbs[3];
} SeeState;

static int see_piece_value(PieceType piece)
{
    switch (piece) {
    case PAWN:
        return HC_PAWN_VALUE;
    case KNIGHT:
        return HC_KNIGHT_VALUE;
    case BISHOP:
        return HC_BISHOP_VALUE;
    case ROOK:
        return HC_ROOK_VALUE;
    case QUEEN:
        return HC_QUEEN_VALUE;
    case KING:
        return HC_KING_VALUE;
    default:
        return 0;
    }
}

static PieceType state_piece_at(const SeeState* state, Square square)
{
    for (PieceType piece = PAWN; piece <= KING; piece++) {
        if (bitboard_is_bit_set(state->piece_bbs[WHITE][piece], square)
            || bitboard_is_bit_set(state->piece_bbs[BLACK][piece], square)) {
            return piece;
        }
    }
    return NO_PIECE;
}

static Color state_piece_color_at(const SeeState* state, Square square)
{
    for (Color color = WHITE; color <= BLACK; color++) {
        for (PieceType piece = PAWN; piece <= KING; piece++) {
            if (bitboard_is_bit_set(state->piece_bbs[color][piece], square)) {
                return color;
            }
        }
    }
    return WHITE;
}

static void state_remove_piece(SeeState* state, Square square, Color color, PieceType piece)
{
    Bitboard mask = bitboard_square_mask(square);
    state->piece_bbs[color][piece] &= ~mask;
    state->occupancy_bbs[color] &= ~mask;
    state->occupancy_bbs[2] &= ~mask;
}

static void state_add_piece(SeeState* state, Square square, Color color, PieceType piece)
{
    Bitboard mask = bitboard_square_mask(square);
    state->piece_bbs[color][piece] |= mask;
    state->occupancy_bbs[color] |= mask;
    state->occupancy_bbs[2] |= mask;
}

static Bitboard state_attackers_to(const SeeState* state, Square square, Color color)
{
    Bitboard attackers = 0;

    attackers
        |= get_pawn_attack_bitboard(square, color_opposite(color)) & state->piece_bbs[color][PAWN];
    attackers |= get_knight_attack_bitboard(square) & state->piece_bbs[color][KNIGHT];
    attackers |= get_king_attack_bitboard(square) & state->piece_bbs[color][KING];
    attackers |= get_bishop_attack_bitboard(square, state->occupancy_bbs[2])
        & (state->piece_bbs[color][BISHOP] | state->piece_bbs[color][QUEEN]);
    attackers |= get_rook_attack_bitboard(square, state->occupancy_bbs[2])
        & (state->piece_bbs[color][ROOK] | state->piece_bbs[color][QUEEN]);

    return attackers;
}

static bool state_king_in_check(const SeeState* state, Color color)
{
    Bitboard king = state->piece_bbs[color][KING];
    if (king == 0) {
        return true;
    }

    Square king_square = (Square)bitboard_lsb_index_unsafe(king);
    return state_attackers_to(state, king_square, color_opposite(color)) != 0;
}

static bool state_apply_capture(const SeeState* state, Color color, Square from, Square to,
                                PieceType promotion, SeeState* result)
{
    *result                = *state;

    PieceType moving_piece = state_piece_at(state, from);
    if (moving_piece == NO_PIECE || state_piece_color_at(state, from) != color) {
        return false;
    }

    PieceType captured_piece = state_piece_at(state, to);
    if (captured_piece != NO_PIECE) {
        Color captured_color = state_piece_color_at(state, to);
        if (captured_color == color) {
            return false;
        }
        state_remove_piece(result, to, captured_color, captured_piece);
    }

    state_remove_piece(result, from, color, moving_piece);
    state_add_piece(result, to, color, promotion == NO_PIECE ? moving_piece : promotion);
    return true;
}

static bool is_promotion_square(Color color, Square square)
{
    return color == WHITE ? square >= A8 : square <= H1;
}

static int promotion_gain(Color color, PieceType moving_piece, Square to, PieceType promotion)
{
    if (moving_piece != PAWN || !is_promotion_square(color, to) || promotion == NO_PIECE) {
        return 0;
    }
    return see_piece_value(promotion) - see_piece_value(PAWN);
}

static bool find_least_valuable_legal_attacker(const SeeState* state, Square target, Color color,
                                               Square* from_out, PieceType* piece_out,
                                               SeeState* next_state)
{
    Bitboard attackers = state_attackers_to(state, target, color);
    while (attackers) {
        Bitboard candidates       = attackers;
        PieceType candidate_piece = NO_PIECE;

        for (PieceType piece = PAWN; piece <= KING; piece++) {
            Bitboard piece_attackers = candidates & state->piece_bbs[color][piece];
            if (piece_attackers) {
                candidate_piece = piece;
                candidates      = piece_attackers;
                break;
            }
        }

        while (candidates) {
            Square from         = (Square)bitboard_pop_lsb_unsafe(&candidates);
            PieceType promotion = NO_PIECE;
            if (candidate_piece == PAWN && is_promotion_square(color, target)) {
                promotion = QUEEN;
            }

            SeeState candidate_state;
            if (!state_apply_capture(state, color, from, target, promotion, &candidate_state)) {
                continue;
            }
            if (!state_king_in_check(&candidate_state, color)) {
                *from_out   = from;
                *piece_out  = candidate_piece;
                *next_state = candidate_state;
                return true;
            }
        }

        /* All attackers of the least valuable type were pinned or illegal. */
        attackers &= ~state->piece_bbs[color][candidate_piece];
    }

    return false;
}

int see_capture(const CBoard* board, Move move)
{
    if (!board || move == MOVE_NONE) {
        return 0;
    }

    Square from   = move_get_from_square(move);
    Square target = move_get_to_square(move);
    if (from == NO_SQUARE || target == NO_SQUARE || move_is_castling(move)) {
        return 0;
    }

    PieceType moving_piece = cboard_get_piece_at_square(board, from);
    if (moving_piece == NO_PIECE) {
        return 0;
    }
    if (!move_is_capture(board, move) && !move_is_promotion(move)) {
        return 0;
    }

    SeeState state = { 0 };
    for (Color color = WHITE; color <= BLACK; color++) {
        for (PieceType piece = NO_PIECE; piece <= KING; piece++) {
            state.piece_bbs[color][piece] = board->piece_bbs[color][piece];
        }
        state.occupancy_bbs[color] = board->occupancy_bbs[color];
    }
    state.occupancy_bbs[2]   = board->occupancy_bbs[2];

    PieceType captured_piece = state_piece_at(&state, target);
    if (captured_piece != NO_PIECE && state_piece_color_at(&state, target) == board->side_to_move) {
        return 0;
    }

    Square captured_square = target;
    if (move_is_enpassant(move)) {
        captured_piece  = PAWN;
        captured_square = (Square)(target + 8 * (2 * board->side_to_move - 1));
    }

    PieceType promotion = move_is_promotion(move) ? move_get_promotion_piecetype(move) : NO_PIECE;
    int gain[64]        = { 0 };
    gain[0]             = see_piece_value(captured_piece)
        + promotion_gain(board->side_to_move, moving_piece, target, promotion);

    if (captured_piece != NO_PIECE) {
        Color captured_color = color_opposite(board->side_to_move);
        if (state_piece_at(&state, captured_square) == captured_piece) {
            state_remove_piece(&state, captured_square, captured_color, captured_piece);
        }
    }

    state_remove_piece(&state, from, board->side_to_move, moving_piece);
    state_add_piece(&state, target, board->side_to_move,
                    promotion == NO_PIECE ? moving_piece : promotion);

    Color side = color_opposite(board->side_to_move);
    int depth  = 0;
    while (depth + 1 < 64) {
        Square attacker_from;
        PieceType attacker_piece;
        SeeState next_state;
        if (!find_least_valuable_legal_attacker(&state, target, side, &attacker_from,
                                                &attacker_piece, &next_state)) {
            break;
        }

        PieceType target_piece       = state_piece_at(&state, target);
        PieceType attacker_promotion = NO_PIECE;
        if (attacker_piece == PAWN && is_promotion_square(side, target)) {
            attacker_promotion = QUEEN;
        }
        int next_depth   = depth + 1;
        gain[next_depth] = see_piece_value(target_piece)
            + promotion_gain(side, attacker_piece, target, attacker_promotion) - gain[depth];
        depth = next_depth;
        state = next_state;
        side  = color_opposite(side);
    }

    for (int index = depth - 1; index >= 0; index--) {
        int best_reply = gain[index + 1] > -gain[index] ? gain[index + 1] : -gain[index];
        gain[index]    = -best_reply;
    }
    return gain[0];
}
