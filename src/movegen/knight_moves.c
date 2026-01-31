#include "movegen/knight_moves.h"
#include "attacks/constant_attacks.h"
#include "core/bitboard.h"

void genAllPseudoLegalKnightMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard knights = (sideToMove == WHITE) ? board->whiteKnights : board->blackKnights;
    Bitboard opponentPieces = (sideToMove == WHITE) ? board->blackPieces : board->whitePieces;

    while (knights)
    {
        Square from = bb_pop_lsb(&knights);
        Bitboard attacks = getKnightAttacks(from);

        // Generate captures
        Bitboard captures = attacks & opponentPieces;
        while (captures)
        {
            Square to = bb_pop_lsb(&captures);
            Move move = MAKE_CAPTURE(from, to);
            moveList->moves[moveList->count++] = move;
        }

        // Generate quiet moves
        Bitboard quietMoves = attacks & ~board->allPieces;
        while (quietMoves)
        {
            Square to = bb_pop_lsb(&quietMoves);
            Move move = MAKE_MOVE(from, to);
            moveList->moves[moveList->count++] = move;
        }
    }
}
