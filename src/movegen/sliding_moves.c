#include "movegen/sliding_moves.h"
#include "attacks/sliding_attacks.h"
#include "core/bitboard.h"

void genAllPseudoLegalBishopMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard bishops = (sideToMove == WHITE) ? board->whiteBishops : board->blackBishops;
    Bitboard opponentPieces = (sideToMove == WHITE) ? board->blackPieces : board->whitePieces;
    while (bishops)
    {
        Square from = bb_pop_lsb(&bishops);
        Bitboard attacks = getBishopAttacks(from, board->allPieces);

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

void genAllPseudoLegalRookMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard rooks = (sideToMove == WHITE) ? board->whiteRooks : board->blackRooks;
    Bitboard opponentPieces = (sideToMove == WHITE) ? board->blackPieces : board->whitePieces;
    while (rooks)
    {
        Square from = bb_pop_lsb(&rooks);
        Bitboard attacks = getRookAttacks(from, board->allPieces);

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

void genAllPseudoLegalQueenMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard queens = (sideToMove == WHITE) ? board->whiteQueens : board->blackQueens;
    Bitboard opponentPieces = (sideToMove == WHITE) ? board->blackPieces : board->whitePieces;
    while (queens)
    {
        Square from = bb_pop_lsb(&queens);
        Bitboard attacks = getQueenAttacks(from, board->allPieces);

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
