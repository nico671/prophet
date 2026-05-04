#include "attacks/sliding_attacks.h"
#include "board/cboard.h"
#include "core/bitboard.h"
#include "movegen/sliding_moves.h"

void gen_all_pseudolegal_bishop_moves(CBoard* board, MoveList* moveList)
{
    Color sideToMove = board->side_to_move;
    Bitboard bishops = (sideToMove == WHITE) ? board->white_bishops_bb : board->black_bishops_bb;
    Bitboard opponentPieces = (sideToMove == WHITE)
        ? (board->black_pieces_bb & ~board->black_king_bb)
        : (board->white_pieces_bb & ~board->white_king_bb);
    while (bishops) {
        Square from = bitboard_pop_lsb(&bishops);
        Bitboard attacks = get_bishop_attack_bitboard(from, board->all_pieces_bb);

        Bitboard captures = attacks & opponentPieces;
        while (captures) {
            Square to = bitboard_pop_lsb(&captures);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->all_pieces_bb;
        while (quietMoves) {
            Square to = bitboard_pop_lsb(&quietMoves);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void gen_all_pseudolegal_rook_moves(CBoard* board, MoveList* moveList)
{
    Color sideToMove = board->side_to_move;
    Bitboard rooks = (sideToMove == WHITE) ? board->white_rooks_bb : board->black_rooks_bb;
    Bitboard opponentPieces = (sideToMove == WHITE)
        ? (board->black_pieces_bb & ~board->black_king_bb)
        : (board->white_pieces_bb & ~board->white_king_bb);
    while (rooks) {
        Square from = bitboard_pop_lsb(&rooks);
        Bitboard attacks = get_rook_attack_bitboard(from, board->all_pieces_bb);

        Bitboard captures = attacks & opponentPieces;
        while (captures) {
            Square to = bitboard_pop_lsb(&captures);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->all_pieces_bb;
        while (quietMoves) {
            Square to = bitboard_pop_lsb(&quietMoves);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void gen_all_pseudolegal_queen_moves(CBoard* board, MoveList* moveList)
{
    Color sideToMove = board->side_to_move;
    Bitboard queens = (sideToMove == WHITE) ? board->white_queens_bb : board->black_queens_bb;
    Bitboard opponentPieces = (sideToMove == WHITE)
        ? (board->black_pieces_bb & ~board->black_king_bb)
        : (board->white_pieces_bb & ~board->white_king_bb);
    while (queens) {
        Square from = bitboard_pop_lsb(&queens);
        Bitboard attacks = get_queen_attack_bitboard(from, board->all_pieces_bb);

        Bitboard captures = attacks & opponentPieces;
        while (captures) {
            Square to = bitboard_pop_lsb(&captures);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->all_pieces_bb;
        while (quietMoves) {
            Square to = bitboard_pop_lsb(&quietMoves);
            Move move = create_move(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }
    }
}
