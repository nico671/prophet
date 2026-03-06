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
                                ? bitboardShiftNorth(pawns) &
                                      emptySquares
                                : bitboardShiftSouth(pawns) &
                                      emptySquares;

    while (singlePushes)
    {
        Square to = bitboardPopLSB(&singlePushes);
        Square from = to - (sideToMove == WHITE ? 8 : -8);
        Move move = createMove(from, to, NORMAL, NO_PIECE);
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
        Bitboard singlePushes = bitboardShiftNorth(pawns) & emptySquares;
        // Second push must also land on empty square
        doublePushes = bitboardShiftNorth(singlePushes) & emptySquares;
    }
    else
    {
        // First push must land on empty square
        Bitboard singlePushes = bitboardShiftSouth(pawns) & emptySquares;
        // Second push must also land on empty square
        doublePushes = bitboardShiftSouth(singlePushes) & emptySquares;
    }

    // Iterate through each square in the double pushes bitboard
    while (doublePushes)
    {
        Square to = bitboardPopLSB(&doublePushes);
        Square from = to - (sideToMove == WHITE ? 16 : -16);
        Move move = createMove(from, to, NORMAL, NO_PIECE);
        moveList->moves[moveList->count++] = move;
    }
}

void genPawnCaptures(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard pawns = (sideToMove == WHITE) ? board->whitePawns : board->blackPawns;
    Bitboard promotionRank = (sideToMove == WHITE) ? RANK_7 : RANK_2;
    pawns &= ~promotionRank; // exclude pawns on promotion rank, handled in promotions function
    Bitboard opponentPieces = (sideToMove == WHITE)
                                  ? (board->blackPieces & ~board->blackKing)
                                  : (board->whitePieces & ~board->whiteKing);

    while (pawns)
    {
        Square from = bitboardPopLSB(&pawns);
        Bitboard captureTargets = getPawnAttacks(from, sideToMove) & opponentPieces;

        while (captureTargets)
        {
            Square to = bitboardPopLSB(&captureTargets);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void genPawnPromotions(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard pawns = (sideToMove == WHITE) ? board->whitePawns : board->blackPawns;
    Bitboard emptySquares = ~(board->allPieces);
    Bitboard opponentPieces = (sideToMove == WHITE)
                                  ? (board->blackPieces & ~board->blackKing)
                                  : (board->whitePieces & ~board->whiteKing);

    Bitboard promotionRank = (sideToMove == WHITE) ? RANK_7 : RANK_2;
    pawns &= promotionRank;

    // Promotion pushes
    Bitboard promotionPushes = (sideToMove == WHITE)
                                   ? bitboardShiftNorth(pawns) &
                                         emptySquares & RANK_8
                                   : bitboardShiftSouth(pawns) & emptySquares & RANK_1;

    while (promotionPushes)
    {
        Square to = bitboardPopLSB(&promotionPushes);
        Square from = to - (sideToMove == WHITE ? 8 : -8);
        // Generate all promotion piece types
        for (PieceType pt = KNIGHT; pt <= QUEEN; pt++)
        {
            Move move = createMove(from, to, PROMO, pt);
            moveList->moves[moveList->count++] = move;
        }
    }

    // Promotion captures
    while (pawns)
    {
        Square from = bitboardPopLSB(&pawns);
        Bitboard captureTargets = getPawnAttacks(from, sideToMove) & opponentPieces;

        while (captureTargets)
        {
            Square to = bitboardPopLSB(&captureTargets);
            // Generate all promotion piece types
            for (PieceType pt = KNIGHT; pt <= QUEEN; pt++)
            {
                Move move = createMove(from, to, PROMO, pt);
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
                             ? getPawnAttacks(board->epSquare, BLACK)
                             : getPawnAttacks(board->epSquare, WHITE);

    Bitboard pawnsThatCanCaptureEP = pawns & attackers;

    while (pawnsThatCanCaptureEP)
    {
        Square from = bitboardPopLSB(&pawnsThatCanCaptureEP);
        Square to = board->epSquare;
        Move move = createMove(from, to, EN_PASSANT, NO_PIECE);
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
