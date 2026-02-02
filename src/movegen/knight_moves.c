#include "movegen/knight_moves.h"
#include "attacks/constant_attacks.h"
#include "core/bitboard.h"
#include "board/cboard.h"

void genAllPseudoLegalKnightMoves(CBoard *board, MoveList *moveList)
{
    Bitboard knights = (board->sideToMove == WHITE) ? board->whiteKnights : board->blackKnights;
    Bitboard friendlyPieces = (board->sideToMove == WHITE) ? board->whitePieces : board->blackPieces;
    Bitboard enemyPieces = (board->sideToMove == WHITE) ? board->blackPieces : board->whitePieces;

    while (knights)
    {
        Square from = bb_pop_lsb(&knights);
        Bitboard attacks = getKnightAttacks(from);

        attacks &= ~friendlyPieces;

        // Separate quiet moves and captures
        Bitboard quietMoves = attacks & ~board->allPieces;
        Bitboard captures = attacks & enemyPieces;

        // Generate quiet moves
        while (quietMoves)
        {
            Square to = bb_pop_lsb(&quietMoves);
            Move move = MAKE_MOVE(from, to);
            moveList->moves[moveList->count++] = move;
        }

        // Generate captures
        while (captures)
        {
            Square to = bb_pop_lsb(&captures);
            Move move = MAKE_CAPTURE(from, to);
            moveList->moves[moveList->count++] = move;
        }
    }
}
