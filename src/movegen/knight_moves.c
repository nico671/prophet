#include "movegen/knight_moves.h"
#include "attacks/constant_attacks.h"
#include "core/bitboard.h"
#include "board/cboard.h"

// Generates all pseudo-legal knight moves (both quiet moves and captures) for the side to move on the given board.
// This function does NOT check for king safety, so it may generate moves that leave the king in check. It is the caller's responsibility to filter those out if necessary.
void genAllPseudoLegalKnightMoves(CBoard *board, MoveList *moveList)
{
    Bitboard knights = (board->sideToMove == WHITE) ? board->whiteKnights : board->blackKnights;
    Bitboard friendlyPieces = (board->sideToMove == WHITE) ? board->whitePieces : board->blackPieces;
    Bitboard enemyPieces = (board->sideToMove == WHITE) ? board->blackPieces : board->whitePieces;

    while (knights)
    {
        Square from = bitboardPopLSB(&knights);
        Bitboard attacks = getKnightAttacks(from);

        attacks &= ~friendlyPieces;

        // Separate quiet moves and captures
        Bitboard quietMoves = attacks & ~board->allPieces;
        Bitboard captures = attacks & enemyPieces;

        // Generate quiet moves
        while (quietMoves)
        {
            Square to = bitboardPopLSB(&quietMoves);
            Move move = MAKE_MOVE(from, to);
            moveList->moves[moveList->count++] = move;
        }

        // Generate captures
        while (captures)
        {
            Square to = bitboardPopLSB(&captures);
            Move move = MAKE_CAPTURE(from, to);
            moveList->moves[moveList->count++] = move;
        }
    }
}
