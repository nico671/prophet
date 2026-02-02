#include "movegen/king_moves.h"
#include "attacks/constant_attacks.h"
#include "core/bitboard.h"
#include "board/cboard.h"

void genAllPseudoLegalKingNonCastlingMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard king = (sideToMove == WHITE) ? board->whiteKing : board->blackKing;
    Bitboard opponentPieces = (sideToMove == WHITE) ? board->blackPieces : board->whitePieces;
    if (king)
    {
        Square from = bb_pop_lsb(&king);
        Bitboard attacks = getKingAttacks(from);

        Bitboard captures = attacks & opponentPieces;
        while (captures)
        {
            Square to = bb_pop_lsb(&captures);
            Move move = MAKE_CAPTURE(from, to);
            moveList->moves[moveList->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->allPieces;
        while (quietMoves)
        {
            Square to = bb_pop_lsb(&quietMoves);
            Move move = MAKE_MOVE(from, to);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void genAllPseudoLegalKingMoves(CBoard *board, MoveList *moveList)
{
    genAllPseudoLegalKingNonCastlingMoves(board, moveList);
    // handle white castling
    if (board->sideToMove == WHITE)
    {
        if (CHECK_BIT(board->castlingRights, 3))
        {
            if ((bb_lsb_idx(board->whiteKing) == E1) &&
                (is_bit_set(board->whiteRooks, H1)) &&
                !(is_bit_set(board->allPieces, F1)) &&
                !(is_bit_set(board->allPieces, G1)))
            {

                Move move = MAKE_CASTLE_KING(E1, G1);
                moveList->moves[moveList->count++] = move;
            }
        }
        if (CHECK_BIT(board->castlingRights, 2))
        {
            if ((bb_lsb_idx(board->whiteKing) == E1) &&
                (is_bit_set(board->whiteRooks, A1)) &&
                !(is_bit_set(board->allPieces, D1)) &&
                !(is_bit_set(board->allPieces, C1)) &&
                !(is_bit_set(board->allPieces, B1)))
            {

                Move move = MAKE_CASTLE_QUEEN(E1, C1);
                moveList->moves[moveList->count++] = move;
            }
        }
    }
    // handle black castling
    else
    {
        if (CHECK_BIT(board->castlingRights, 1))
        {
            if ((bb_lsb_idx(board->blackKing) == E8) &&
                (is_bit_set(board->blackRooks, H8)) &&
                !(is_bit_set(board->allPieces, F8)) &&
                !(is_bit_set(board->allPieces, G8)))
            {

                Move move = MAKE_CASTLE_KING(E8, G8);
                moveList->moves[moveList->count++] = move;
            }
        }
        if (CHECK_BIT(board->castlingRights, 0))
        {
            if ((bb_lsb_idx(board->blackKing) == E8) &&
                (is_bit_set(board->blackRooks, A8)) &&
                !(is_bit_set(board->allPieces, D8)) &&
                !(is_bit_set(board->allPieces, C8)) &&
                !(is_bit_set(board->allPieces, B8)))
            {

                Move move = MAKE_CASTLE_QUEEN(E8, C8);
                moveList->moves[moveList->count++] = move;
            }
        }
    }
}
