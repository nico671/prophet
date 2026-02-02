#include "movegen/pawn_moves.h"
#include "attacks/constant_attacks.h"
#include "core/bitboard.h"
#include "board/cboard.h"

void genSinglePawnPushes(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard pawns = (sideToMove == WHITE) ? board->whitePawns : board->blackPawns;
    Bitboard emptySquares = ~(board->allPieces);
    Bitboard promotionRank = (sideToMove == WHITE) ? RANK_7 : RANK_2;
    pawns &= ~promotionRank; // exclude pawns on promotion rank
    Bitboard singlePushes = (sideToMove == WHITE)
                                ? north(pawns) & emptySquares
                                : south(pawns) & emptySquares;

    while (singlePushes)
    {
        Square to = bb_pop_lsb(&singlePushes);
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
        Square to = bb_pop_lsb(&doublePushes);
        Square from = to - (sideToMove == WHITE ? 16 : -16);
        Move move = MAKE_DOUBLE_PUSH(from, to);
        moveList->moves[moveList->count++] = move;
    }
}

void genPawnCaptures(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard pawns = (sideToMove == WHITE) ? board->whitePawns : board->blackPawns;
    Bitboard promotionRank = (sideToMove == WHITE) ? RANK_7 : RANK_2;
    pawns &= ~promotionRank; // exclude pawns on promotion rank, handled in promotions function
    Bitboard opponentPieces = (sideToMove == WHITE) ? board->blackPieces : board->whitePieces;

    while (pawns)
    {
        Square from = bb_pop_lsb(&pawns);
        Bitboard captureTargets = (sideToMove == WHITE)
                                      ? getWhitePawnAttacks(from) & opponentPieces
                                      : getBlackPawnAttacks(from) & opponentPieces;

        while (captureTargets)
        {
            Square to = bb_pop_lsb(&captureTargets);
            Move move = MAKE_CAPTURE(from, to);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void genPawnPromotions(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard pawns = (sideToMove == WHITE) ? board->whitePawns : board->blackPawns;
    Bitboard emptySquares = ~(board->allPieces);
    Bitboard opponentPieces = (sideToMove == WHITE) ? board->blackPieces : board->whitePieces;

    Bitboard promotionRank = (sideToMove == WHITE) ? RANK_7 : RANK_2;
    pawns &= promotionRank;

    // Promotion pushes
    Bitboard promotionPushes = (sideToMove == WHITE)
                                   ? north(pawns) & emptySquares & RANK_8
                                   : south(pawns) & emptySquares & RANK_1;

    while (promotionPushes)
    {
        Square to = bb_pop_lsb(&promotionPushes);
        Square from = to - (sideToMove == WHITE ? 8 : -8);
        // Generate all promotion piece types
        for (PieceType pt = KNIGHT; pt <= QUEEN; pt++)
        {
            Move move = MAKE_PROMOTION(from, to, pt, false);
            moveList->moves[moveList->count++] = move;
        }
    }

    // Promotion captures
    while (pawns)
    {
        Square from = bb_pop_lsb(&pawns);
        Bitboard captureTargets = (sideToMove == WHITE)
                                      ? getWhitePawnAttacks(from) & opponentPieces
                                      : getBlackPawnAttacks(from) & opponentPieces;

        while (captureTargets)
        {
            Square to = bb_pop_lsb(&captureTargets);
            // Generate all promotion piece types
            for (PieceType pt = KNIGHT; pt <= QUEEN; pt++)
            {
                Move move = MAKE_PROMOTION(from, to, pt, true);
                moveList->moves[moveList->count++] = move;
            }
        }
    }
}

void genEnPassantPawnMoves(CBoard *board, MoveList *moveList)
{
    if (board->epSquare == NO_SQUARE)
        return;

    Color sideToMove = board->sideToMove;
    Bitboard pawns = (sideToMove == WHITE) ? board->whitePawns : board->blackPawns;

    // Get squares that can attack the EP square
    // We need to find which squares our pawns attack FROM to reach epSquare
    // If white to move, we need squares that BLACK pawns would attack from (diagonal down)
    // If black to move, we need squares that WHITE pawns would attack from (diagonal up)
    Bitboard attackers = (sideToMove == WHITE)
                             ? getBlackPawnAttacks(board->epSquare)
                             : getWhitePawnAttacks(board->epSquare);

    Bitboard pawnsThatCanCaptureEP = pawns & attackers;

    while (pawnsThatCanCaptureEP)
    {
        Square from = bb_pop_lsb(&pawnsThatCanCaptureEP);
        Square to = board->epSquare;
        Move move = MAKE_EP(from, to);
        moveList->moves[moveList->count++] = move;
    }
}

void genAllPseudoLegalPawnMoves(CBoard *board, MoveList *moveList)
{
    genSinglePawnPushes(board, moveList);
    genDoublePawnPushes(board, moveList);
    genPawnCaptures(board, moveList);
    genPawnPromotions(board, moveList);
    if (board->epSquare != NO_SQUARE)
        genEnPassantPawnMoves(board, moveList);
}
