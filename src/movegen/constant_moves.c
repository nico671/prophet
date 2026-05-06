#include "attacks/constant_attacks.h"
#include "board/cboard.h"
#include "core/bitboard.h"
#include "movegen/constant_moves.h"
#include "movegen/move.h"

void gen_all_pseudolegal_king_noncastling_moves(CBoard* board, MoveList* move_list)
{
    Color side_to_move = board->side_to_move;
    Bitboard side_to_move_king_bb = board->piece_bbs[side_to_move][KING];
    Bitboard opponent_pieces_bb = board->occupancy_bbs[1 - side_to_move] & ~(board->piece_bbs[1 - side_to_move][KING]);
    if (side_to_move_king_bb) {
        Square from = bitboard_pop_lsb_unsafe(&side_to_move_king_bb);
        Bitboard king_attacks_bb = get_king_attack_bitboard(from);

        Bitboard king_captures_bb = king_attacks_bb & opponent_pieces_bb;
        while (king_captures_bb) {
            Square to = bitboard_pop_lsb_unsafe(&king_captures_bb);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            move_list->moves[move_list->count++] = move;
        }

        Bitboard king_quite_moves_bb = king_attacks_bb & ~(board->occupancy_bbs[2]);
        while (king_quite_moves_bb) {
            Square to = bitboard_pop_lsb_unsafe(&king_quite_moves_bb);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            move_list->moves[move_list->count++] = move;
        }
    }
}

void gen_all_pseudolegal_king_castling_moves(CBoard* board, MoveList* move_list)
{

    // handle white castling
    if (board->side_to_move == WHITE) {
        if (!board->piece_bbs[WHITE][KING]) {
            return;
        }

        if (U8_CHECK_BIT(board->castling_rights, 3)) {
            if ((bitboard_lsb_index_unsafe(board->piece_bbs[WHITE][KING]) == E1) && (bitboard_is_bit_set(board->piece_bbs[WHITE][ROOK], H1)) && !(bitboard_is_bit_set(board->occupancy_bbs[2], F1)) && !(bitboard_is_bit_set(board->occupancy_bbs[2], G1))) {

                Move move = create_move(E1, G1, CASTLE, NO_PIECE);
                move_list->moves[move_list->count++] = move;
            }
        }
        if (U8_CHECK_BIT(board->castling_rights, 2)) {
            if ((bitboard_lsb_index_unsafe(board->piece_bbs[WHITE][KING]) == E1) && (bitboard_is_bit_set(board->piece_bbs[WHITE][ROOK], A1)) && !(bitboard_is_bit_set(board->occupancy_bbs[2], D1)) && !(bitboard_is_bit_set(board->occupancy_bbs[2], C1)) && !(bitboard_is_bit_set(board->occupancy_bbs[2], B1))) {

                Move move = create_move(E1, C1, CASTLE, NO_PIECE);
                move_list->moves[move_list->count++] = move;
            }
        }
    }
    // handle black castling
    else {
        if (!board->piece_bbs[BLACK][KING]) {
            return;
        }

        if (U8_CHECK_BIT(board->castling_rights, 1)) {
            if ((bitboard_lsb_index_unsafe(board->piece_bbs[BLACK][KING]) == E8) && (bitboard_is_bit_set(board->piece_bbs[BLACK][ROOK], H8)) && !(bitboard_is_bit_set(board->occupancy_bbs[2], F8)) && !(bitboard_is_bit_set(board->occupancy_bbs[2], G8))) {

                Move move = create_move(E8, G8, CASTLE, NO_PIECE);
                move_list->moves[move_list->count++] = move;
            }
        }
        if (U8_CHECK_BIT(board->castling_rights, 0)) {
            if ((bitboard_lsb_index_unsafe(board->piece_bbs[BLACK][KING]) == E8) && (bitboard_is_bit_set(board->piece_bbs[BLACK][ROOK], A8)) && !(bitboard_is_bit_set(board->occupancy_bbs[2], D8)) && !(bitboard_is_bit_set(board->occupancy_bbs[2], C8)) && !(bitboard_is_bit_set(board->occupancy_bbs[2], B8))) {

                Move move = create_move(E8, C8, CASTLE, NO_PIECE);
                move_list->moves[move_list->count++] = move;
            }
        }
    }
}

void gen_all_pseudolegal_king_moves(CBoard* board, MoveList* move_list)
{
    gen_all_pseudolegal_king_noncastling_moves(board, move_list);
    gen_all_pseudolegal_king_castling_moves(board, move_list);
}

// Generates all pseudo-legal knight moves (both quiet moves and captures) for the side to move on the given board.
// This function does NOT check for side_to_move_king_bb safety, so it may generate moves that leave the side_to_move_king_bb in check. It is the caller's responsibility to filter those out if necessary.
void gen_all_pseudolegal_knight_moves(CBoard* board, MoveList* move_list)
{
    Bitboard side_to_move_knights_bb = board->piece_bbs[board->side_to_move][KNIGHT];
    Bitboard friendly_pieces_bb = board->occupancy_bbs[board->side_to_move];
    Bitboard enemy_pieces_bb = board->occupancy_bbs[1 - board->side_to_move] & ~(board->piece_bbs[1 - board->side_to_move][KING]);

    while (side_to_move_knights_bb) {
        Square from = bitboard_pop_lsb_unsafe(&side_to_move_knights_bb);
        Bitboard knight_attacks_bb = get_knight_attack_bitboard(from);

        knight_attacks_bb &= ~friendly_pieces_bb;

        // Separate quiet moves and captures
        Bitboard knight_quite_moves_bb = knight_attacks_bb & ~board->occupancy_bbs[2];
        Bitboard knight_captures_bb = knight_attacks_bb & enemy_pieces_bb;

        // Generate quiet moves
        while (knight_quite_moves_bb) {
            Square to = bitboard_pop_lsb_unsafe(&knight_quite_moves_bb);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            move_list->moves[move_list->count++] = move;
        }

        // Generate captures
        while (knight_captures_bb) {
            Square to = bitboard_pop_lsb_unsafe(&knight_captures_bb);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            move_list->moves[move_list->count++] = move;
        }
    }
}

void gen_all_pseudolegal_single_pawn_pushes(CBoard* board, MoveList* move_list)
{
    Color side_to_move = board->side_to_move;
    Bitboard side_to_move_pawns_bb = board->piece_bbs[side_to_move][PAWN];
    Bitboard empty_squares_bb = ~(board->occupancy_bbs[2]);
    Bitboard promotion_rank_bb = (side_to_move == WHITE) ? RANK_7 : RANK_2;
    side_to_move_pawns_bb &= ~promotion_rank_bb; // exclude side_to_move_pawns_bb on promotion rank
    Bitboard single_pawn_pushes_bb = (side_to_move == WHITE)
        ? bitboard_shift_north(side_to_move_pawns_bb) & empty_squares_bb
        : bitboard_shift_south(side_to_move_pawns_bb) & empty_squares_bb;

    while (single_pawn_pushes_bb) {
        Square to = bitboard_pop_lsb_unsafe(&single_pawn_pushes_bb);
        Square from = to + (8 * (2 * board->side_to_move - 1));
        Move move = create_move(from, to, NORMAL, NO_PIECE);
        move_list->moves[move_list->count++] = move;
    }
}

void gen_all_pseudolegal_double_pawn_pushes(CBoard* board, MoveList* move_list)
{
    Color side_to_move = board->side_to_move;
    Bitboard side_to_move_pawns_bb = board->piece_bbs[side_to_move][PAWN];
    Bitboard empty_squares_bb = ~(board->occupancy_bbs[2]);

    // Only consider side_to_move_pawns_bb on their starting rank
    side_to_move_pawns_bb &= (side_to_move == WHITE) ? RANK_2 : RANK_7;

    Bitboard double_pawn_pushes_bb;
    if (side_to_move == WHITE) {
        // First push must land on empty square
        Bitboard single_pawn_pushes_bb = bitboard_shift_north(side_to_move_pawns_bb) & empty_squares_bb;
        // Second push must also land on empty square
        double_pawn_pushes_bb = bitboard_shift_north(single_pawn_pushes_bb) & empty_squares_bb;
    } else {
        // First push must land on empty square
        Bitboard single_pawn_pushes_bb = bitboard_shift_south(side_to_move_pawns_bb) & empty_squares_bb;
        // Second push must also land on empty square
        double_pawn_pushes_bb = bitboard_shift_south(single_pawn_pushes_bb) & empty_squares_bb;
    }

    // Iterate through each square in the double pushes bitboard
    while (double_pawn_pushes_bb) {
        Square to = bitboard_pop_lsb_unsafe(&double_pawn_pushes_bb);
        Square from = to + (16 * (2 * board->side_to_move - 1));
        ;
        Move move = create_move(from, to, NORMAL, NO_PIECE);
        move_list->moves[move_list->count++] = move;
    }
}

void gen_all_pseudolegal_pawn_captures(CBoard* board, MoveList* move_list)
{
    Color side_to_move = board->side_to_move;
    Bitboard side_to_move_pawns_bb = board->piece_bbs[side_to_move][PAWN];
    Bitboard promotion_rank_bb = (side_to_move == WHITE) ? RANK_7 : RANK_2;
    side_to_move_pawns_bb &= ~promotion_rank_bb; // exclude side_to_move_pawns_bb on promotion rank, handled in promotions function
    Bitboard opponent_pieces_bb = board->occupancy_bbs[1 - side_to_move] & ~(board->piece_bbs[1 - side_to_move][KING]);

    while (side_to_move_pawns_bb) {
        Square from = bitboard_pop_lsb_unsafe(&side_to_move_pawns_bb);
        Bitboard capture_targets_bb = get_pawn_attack_bitboard(from, side_to_move) & opponent_pieces_bb;

        while (capture_targets_bb) {
            Square to = bitboard_pop_lsb_unsafe(&capture_targets_bb);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            move_list->moves[move_list->count++] = move;
        }
    }
}

void gen_all_pseudolegal_pawn_promotions(CBoard* board, MoveList* move_list)
{
    Color side_to_move = board->side_to_move;
    Bitboard side_to_move_pawns_bb = board->piece_bbs[side_to_move][PAWN];
    Bitboard empty_squares_bb = ~(board->occupancy_bbs[2]);
    Bitboard opponent_pieces_bb = board->occupancy_bbs[1 - side_to_move] & ~(board->piece_bbs[1 - side_to_move][KING]);

    Bitboard promotion_rank_bb = (side_to_move == WHITE) ? RANK_7 : RANK_2;
    side_to_move_pawns_bb &= promotion_rank_bb;

    // Promotion pushes
    Bitboard promotion_pushes_bb = (side_to_move == WHITE)
        ? bitboard_shift_north(side_to_move_pawns_bb) & empty_squares_bb & RANK_8
        : bitboard_shift_south(side_to_move_pawns_bb) & empty_squares_bb & RANK_1;

    while (promotion_pushes_bb) {
        Square to = bitboard_pop_lsb_unsafe(&promotion_pushes_bb);
        Square from = to + (8 * (2 * board->side_to_move - 1));
        // Generate all promotion piece types
        for (PieceType pt = KNIGHT; pt <= QUEEN; pt++) {
            Move move = create_move(from, to, PROMO, pt);
            move_list->moves[move_list->count++] = move;
        }
    }

    // Promotion captures
    while (side_to_move_pawns_bb) {
        Square from = bitboard_pop_lsb_unsafe(&side_to_move_pawns_bb);
        Bitboard capture_targets_bb = get_pawn_attack_bitboard(from, side_to_move) & opponent_pieces_bb;

        while (capture_targets_bb) {
            Square to = bitboard_pop_lsb_unsafe(&capture_targets_bb);
            // Generate all promotion piece types
            for (PieceType pt = KNIGHT; pt <= QUEEN; pt++) {
                Move move = create_move(from, to, PROMO, pt);
                move_list->moves[move_list->count++] = move;
            }
        }
    }
}

void gen_all_pseudolegal_ep_pawn_moves(CBoard* board, MoveList* move_list)
{
    if (board->ep_square == NO_SQUARE)
        return;

    Color side_to_move = board->side_to_move;
    Bitboard side_to_move_pawns_bb = board->piece_bbs[side_to_move][PAWN];

    // Get squares that can attack the EP square
    // We need to find which squares our side_to_move_pawns_bb attack FROM to reach epSquare
    // If white to move, we need squares that BLACK side_to_move_pawns_bb would attack from (diagonal down)
    // If black to move, we need squares that WHITE side_to_move_pawns_bb would attack from (diagonal up)
    Bitboard attackers_bb = (side_to_move == WHITE)
        ? get_pawn_attack_bitboard(board->ep_square, BLACK)
        : get_pawn_attack_bitboard(board->ep_square, WHITE);

    Bitboard pawns_can_capture_ep_bb = side_to_move_pawns_bb & attackers_bb;

    while (pawns_can_capture_ep_bb) {
        Square from = bitboard_pop_lsb_unsafe(&pawns_can_capture_ep_bb);
        Square to = board->ep_square;
        Move move = create_move(from, to, EN_PASSANT, NO_PIECE);
        move_list->moves[move_list->count++] = move;
    }
}

void gen_all_pseudolegal_pawn_moves(CBoard* board, MoveList* move_list)
{
    gen_all_pseudolegal_single_pawn_pushes(board, move_list);
    gen_all_pseudolegal_double_pawn_pushes(board, move_list);
    gen_all_pseudolegal_pawn_captures(board, move_list);
    gen_all_pseudolegal_pawn_promotions(board, move_list);
    if (board->ep_square != NO_SQUARE) {
        gen_all_pseudolegal_ep_pawn_moves(board, move_list);
    }
}
