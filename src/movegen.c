#include "cboard.h"

void genSinglePawnPushes(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard pawns = (sideToMove == WHITE) ? board->whitePawns : board->blackPawns;
    Bitboard emptySquares = ~(board->allPieces);

    Bitboard singlePushes = (sideToMove == WHITE)
                                ? north(pawns) & emptySquares
                                : south(pawns) & emptySquares;

    while (singlePushes)
    {
        Square to = popLSB(&singlePushes);
        Square from = to - (sideToMove == WHITE ? 8 : -8);
        Move move = MAKE_MOVE(from, to);
        moveList->moves[moveList->count++] = move;
    }
}

void genDoublePawnPushes(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard pawns = (sideToMove == WHITE) ? board->whitePawns : board->blackPawns;
    Bitboard emptySquares = ~(board->allPieces);

    // Only consider pawns on their starting rank
    pawns &= (sideToMove == WHITE) ? RANK_2 : RANK_7;

    Bitboard doublePushes;
    if (sideToMove == WHITE)
    {
        // First push must land on empty square
        Bitboard singlePushes = north(pawns) & emptySquares;
        // Second push must also land on empty square
        doublePushes = north(singlePushes) & emptySquares;
    }
    else
    {
        // First push must land on empty square
        Bitboard singlePushes = south(pawns) & emptySquares;
        // Second push must also land on empty square
        doublePushes = south(singlePushes) & emptySquares;
    }

    // Iterate through each square in the double pushes bitboard
    while (doublePushes)
    {
        Square to = popLSB(&doublePushes);
        Square from = to - (sideToMove == WHITE ? 16 : -16);
        Move move = MAKE_DOUBLE_PUSH(from, to);
        moveList->moves[moveList->count++] = move;
    }
}

void genAllPawnPushes(CBoard *board, MoveList *moveList)
{

    genSinglePawnPushes(board, moveList);
    genDoublePawnPushes(board, moveList);
}