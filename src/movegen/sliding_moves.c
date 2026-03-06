#include "movegen/sliding_moves.h"
#include "attacks/sliding_attacks.h"
#include "core/bitboard.h"
#include "board/cboard.h"

void genAllPseudoLegalBishopMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard bishops = (sideToMove == WHITE) ? board->whiteBishops : board->blackBishops;
    Bitboard opponentPieces = (sideToMove == WHITE)
                                  ? (board->blackPieces & ~board->blackKing)
                                  : (board->whitePieces & ~board->whiteKing);
    while (bishops)
    {
        Square from = bitboardPopLSB(&bishops);
        Bitboard attacks = getBishopAttacks(from, board->allPieces);

        Bitboard captures = attacks & opponentPieces;
        while (captures)
        {
            Square to = bitboardPopLSB(&captures);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->allPieces;
        while (quietMoves)
        {
            Square to = bitboardPopLSB(&quietMoves);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void genAllPseudoLegalRookMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard rooks = (sideToMove == WHITE) ? board->whiteRooks : board->blackRooks;
    Bitboard opponentPieces = (sideToMove == WHITE)
                                  ? (board->blackPieces & ~board->blackKing)
                                  : (board->whitePieces & ~board->whiteKing);
    while (rooks)
    {
        Square from = bitboardPopLSB(&rooks);
        Bitboard attacks = getRookAttacks(from, board->allPieces);

        Bitboard captures = attacks & opponentPieces;
        while (captures)
        {
            Square to = bitboardPopLSB(&captures);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->allPieces;
        while (quietMoves)
        {
            Square to = bitboardPopLSB(&quietMoves);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void genAllPseudoLegalQueenMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard queens = (sideToMove == WHITE) ? board->whiteQueens : board->blackQueens;
    Bitboard opponentPieces = (sideToMove == WHITE)
                                  ? (board->blackPieces & ~board->blackKing)
                                  : (board->whitePieces & ~board->whiteKing);
    while (queens)
    {
        Square from = bitboardPopLSB(&queens);
        Bitboard attacks = getQueenAttacks(from, board->allPieces);

        Bitboard captures = attacks & opponentPieces;
        while (captures)
        {
            Square to = bitboardPopLSB(&captures);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->allPieces;
        while (quietMoves)
        {
            Square to = bitboardPopLSB(&quietMoves);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }
    }
}
