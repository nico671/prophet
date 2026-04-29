#include "attacks/constant_attacks.h"
#include "board/cboard.h"
#include "core/bitboard.h"
#include "movegen/constant_moves.h"
#include "movegen/move.h"

#pragma mark region King Move Generation
void genAllPseudoLegalKingNonCastlingMoves(CBoard* board, MoveList* moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard king = (sideToMove == WHITE) ? board->whiteKing : board->blackKing;
    Bitboard opponentPieces = (sideToMove == WHITE)
        ? (board->blackPieces & ~board->blackKing)
        : (board->whitePieces & ~board->whiteKing);
    if (king) {
        Square from = bitboardPopLSB(&king);
        Bitboard attacks = getKingAttacks(from);

        Bitboard captures = attacks & opponentPieces;
        while (captures) {
            Square to = bitboardPopLSB(&captures);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->allPieces;
        while (quietMoves) {
            Square to = bitboardPopLSB(&quietMoves);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void genAllPseudoLegalKingCastlingMoves(CBoard* board, MoveList* moveList)
{

    // handle white castling
    if (board->sideToMove == WHITE) {
        if (!board->whiteKing) {
            return;
        }

        if (CHECK_BIT(board->castlingRights, 3)) {
            if ((bitboardLSBIndex(board->whiteKing) == E1) && (bitboardIsBitSet(board->whiteRooks, H1)) && !(bitboardIsBitSet(board->allPieces, F1)) && !(bitboardIsBitSet(board->allPieces, G1))) {

                Move move = createMove(E1, G1, CASTLE, NO_PIECE);
                moveList->moves[moveList->count++] = move;
            }
        }
        if (CHECK_BIT(board->castlingRights, 2)) {
            if ((bitboardLSBIndex(board->whiteKing) == E1) && (bitboardIsBitSet(board->whiteRooks, A1)) && !(bitboardIsBitSet(board->allPieces, D1)) && !(bitboardIsBitSet(board->allPieces, C1)) && !(bitboardIsBitSet(board->allPieces, B1))) {

                Move move = createMove(E1, C1, CASTLE, NO_PIECE);
                moveList->moves[moveList->count++] = move;
            }
        }
    }
    // handle black castling
    else {
        if (!board->blackKing) {
            return;
        }

        if (CHECK_BIT(board->castlingRights, 1)) {
            if ((bitboardLSBIndex(board->blackKing) == E8) && (bitboardIsBitSet(board->blackRooks, H8)) && !(bitboardIsBitSet(board->allPieces, F8)) && !(bitboardIsBitSet(board->allPieces, G8))) {

                Move move = createMove(E8, G8, CASTLE, NO_PIECE);
                moveList->moves[moveList->count++] = move;
            }
        }
        if (CHECK_BIT(board->castlingRights, 0)) {
            if ((bitboardLSBIndex(board->blackKing) == E8) && (bitboardIsBitSet(board->blackRooks, A8)) && !(bitboardIsBitSet(board->allPieces, D8)) && !(bitboardIsBitSet(board->allPieces, C8)) && !(bitboardIsBitSet(board->allPieces, B8))) {

                Move move = createMove(E8, C8, CASTLE, NO_PIECE);
                moveList->moves[moveList->count++] = move;
            }
        }
    }
}

void genAllPseudoLegalKingMoves(CBoard* board, MoveList* moveList)
{
    genAllPseudoLegalKingNonCastlingMoves(board, moveList);
    genAllPseudoLegalKingCastlingMoves(board, moveList);
}

// Generates all pseudo-legal knight moves (both quiet moves and captures) for the side to move on the given board.
// This function does NOT check for king safety, so it may generate moves that leave the king in check. It is the caller's responsibility to filter those out if necessary.
void genAllPseudoLegalKnightMoves(CBoard* board, MoveList* moveList)
{
    Bitboard knights = (board->sideToMove == WHITE) ? board->whiteKnights : board->blackKnights;
    Bitboard friendlyPieces = (board->sideToMove == WHITE) ? board->whitePieces : board->blackPieces;
    Bitboard enemyPieces = (board->sideToMove == WHITE)
        ? (board->blackPieces & ~board->blackKing)
        : (board->whitePieces & ~board->whiteKing);

    while (knights) {
        Square from = bitboardPopLSB(&knights);
        Bitboard attacks = getKnightAttacks(from);

        attacks &= ~friendlyPieces;

        // Separate quiet moves and captures
        Bitboard quietMoves = attacks & ~board->allPieces;
        Bitboard captures = attacks & enemyPieces;

        // Generate quiet moves
        while (quietMoves) {
            Square to = bitboardPopLSB(&quietMoves);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }

        // Generate captures
        while (captures) {
            Square to = bitboardPopLSB(&captures);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void genSinglePawnPushes(CBoard* board, MoveList* moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard pawns = (sideToMove == WHITE) ? board->whitePawns : board->blackPawns;
    Bitboard emptySquares = ~(board->allPieces);
    Bitboard promotionRank = (sideToMove == WHITE) ? RANK_7 : RANK_2;
    pawns &= ~promotionRank; // exclude pawns on promotion rank
    Bitboard singlePushes = (sideToMove == WHITE)
        ? bitboardShiftNorth(pawns) & emptySquares
        : bitboardShiftSouth(pawns) & emptySquares;

    while (singlePushes) {
        Square to = bitboardPopLSB(&singlePushes);
        Square from = to - (sideToMove == WHITE ? 8 : -8);
        Move move = createMove(from, to, NORMAL, NO_PIECE);
        moveList->moves[moveList->count++] = move;
    }
}

void genDoublePawnPushes(CBoard* board, MoveList* moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard pawns = (sideToMove == WHITE) ? board->whitePawns : board->blackPawns;
    Bitboard emptySquares = ~(board->allPieces);

    // Only consider pawns on their starting rank
    pawns &= (sideToMove == WHITE) ? RANK_2 : RANK_7;

    Bitboard doublePushes;
    if (sideToMove == WHITE) {
        // First push must land on empty square
        Bitboard singlePushes = bitboardShiftNorth(pawns) & emptySquares;
        // Second push must also land on empty square
        doublePushes = bitboardShiftNorth(singlePushes) & emptySquares;
    } else {
        // First push must land on empty square
        Bitboard singlePushes = bitboardShiftSouth(pawns) & emptySquares;
        // Second push must also land on empty square
        doublePushes = bitboardShiftSouth(singlePushes) & emptySquares;
    }

    // Iterate through each square in the double pushes bitboard
    while (doublePushes) {
        Square to = bitboardPopLSB(&doublePushes);
        Square from = to - (sideToMove == WHITE ? 16 : -16);
        Move move = createMove(from, to, NORMAL, NO_PIECE);
        moveList->moves[moveList->count++] = move;
    }
}

void genPawnCaptures(CBoard* board, MoveList* moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard pawns = (sideToMove == WHITE) ? board->whitePawns : board->blackPawns;
    Bitboard promotionRank = (sideToMove == WHITE) ? RANK_7 : RANK_2;
    pawns &= ~promotionRank; // exclude pawns on promotion rank, handled in promotions function
    Bitboard opponentPieces = (sideToMove == WHITE)
        ? (board->blackPieces & ~board->blackKing)
        : (board->whitePieces & ~board->whiteKing);

    while (pawns) {
        Square from = bitboardPopLSB(&pawns);
        Bitboard captureTargets = getPawnAttacks(from, sideToMove) & opponentPieces;

        while (captureTargets) {
            Square to = bitboardPopLSB(&captureTargets);
            Move move = createMove(from, to, NORMAL, NO_PIECE);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void genPawnPromotions(CBoard* board, MoveList* moveList)
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
        ? bitboardShiftNorth(pawns) & emptySquares & RANK_8
        : bitboardShiftSouth(pawns) & emptySquares & RANK_1;

    while (promotionPushes) {
        Square to = bitboardPopLSB(&promotionPushes);
        Square from = to - (sideToMove == WHITE ? 8 : -8);
        // Generate all promotion piece types
        for (PieceType pt = KNIGHT; pt <= QUEEN; pt++) {
            Move move = createMove(from, to, PROMO, pt);
            moveList->moves[moveList->count++] = move;
        }
    }

    // Promotion captures
    while (pawns) {
        Square from = bitboardPopLSB(&pawns);
        Bitboard captureTargets = getPawnAttacks(from, sideToMove) & opponentPieces;

        while (captureTargets) {
            Square to = bitboardPopLSB(&captureTargets);
            // Generate all promotion piece types
            for (PieceType pt = KNIGHT; pt <= QUEEN; pt++) {
                Move move = createMove(from, to, PROMO, pt);
                moveList->moves[moveList->count++] = move;
            }
        }
    }
}

void genEnPassantPawnMoves(CBoard* board, MoveList* moveList)
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

    while (pawnsThatCanCaptureEP) {
        Square from = bitboardPopLSB(&pawnsThatCanCaptureEP);
        Square to = board->epSquare;
        Move move = createMove(from, to, EN_PASSANT, NO_PIECE);
        moveList->moves[moveList->count++] = move;
    }
}

void genAllPseudoLegalPawnMoves(CBoard* board, MoveList* moveList)
{
    genSinglePawnPushes(board, moveList);
    genDoublePawnPushes(board, moveList);
    genPawnCaptures(board, moveList);
    genPawnPromotions(board, moveList);
    if (board->epSquare != NO_SQUARE)
        genEnPassantPawnMoves(board, moveList);
}
