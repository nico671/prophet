#include "board/cboard.h"
#include "core/bitboard.h"
#include "movegen/sliding_attacks.h"
#include "movegen/sliding_moves.h"

void gen_all_pseudolegal_bishop_moves(CBoard* board, MoveList* move_list)
{
    Color sideToMove = board->side_to_move;
    Bitboard bishops = board->piece_bbs[sideToMove][BISHOP];
    Bitboard opponentPieces = board->occupancy_bbs[1 - sideToMove] & ~(board->piece_bbs[1 - sideToMove][KING]);
    while (bishops) {
        Square from = bitboard_pop_lsb_unsafe(&bishops);
        Bitboard attacks = get_bishop_attack_bitboard(from, board->occupancy_bbs[2]);

        Bitboard captures = attacks & opponentPieces;
        while (captures) {
            Square to = bitboard_pop_lsb_unsafe(&captures);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            move_list->moves[move_list->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->occupancy_bbs[2];
        while (quietMoves) {
            Square to = bitboard_pop_lsb_unsafe(&quietMoves);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            move_list->moves[move_list->count++] = move;
        }
    }
}

void gen_all_pseudolegal_rook_moves(CBoard* board, MoveList* move_list)
{
    Color sideToMove = board->side_to_move;
    Bitboard rooks = board->piece_bbs[sideToMove][ROOK];
    Bitboard opponentPieces = board->occupancy_bbs[1 - sideToMove] & ~(board->piece_bbs[1 - sideToMove][KING]);
    while (rooks) {
        Square from = bitboard_pop_lsb_unsafe(&rooks);
        Bitboard attacks = get_rook_attack_bitboard(from, board->occupancy_bbs[2]);

        Bitboard captures = attacks & opponentPieces;
        while (captures) {
            Square to = bitboard_pop_lsb_unsafe(&captures);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            move_list->moves[move_list->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->occupancy_bbs[2];
        while (quietMoves) {
            Square to = bitboard_pop_lsb_unsafe(&quietMoves);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            move_list->moves[move_list->count++] = move;
        }
    }
}

void gen_all_pseudolegal_queen_moves(CBoard* board, MoveList* move_list)
{
    Color sideToMove = board->side_to_move;
    Bitboard queens = board->piece_bbs[sideToMove][QUEEN];
    Bitboard opponentPieces = board->occupancy_bbs[1 - sideToMove] & ~(board->piece_bbs[1 - sideToMove][KING]);
    while (queens) {
        Square from = bitboard_pop_lsb_unsafe(&queens);
        Bitboard attacks = get_queen_attack_bitboard(from, board->occupancy_bbs[2]);

        Bitboard captures = attacks & opponentPieces;
        while (captures) {
            Square to = bitboard_pop_lsb_unsafe(&captures);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            move_list->moves[move_list->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->occupancy_bbs[2];
        while (quietMoves) {
            Square to = bitboard_pop_lsb_unsafe(&quietMoves);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            move_list->moves[move_list->count++] = move;
        }
    }
}
