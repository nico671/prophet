#include "cboard.h"
#include "movegen.h"
#include "constant_attacks.h"
#include "bitboard.h"
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
            Move move = MAKE_PROMOTION(from, to, pt);
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
                Move move = MAKE_PROMOTION_CAPTURE(from, to, pt);
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
    // If white to move, use white pawn attacks (find which squares white pawns attack FROM)
    // If black to move, use black pawn attacks (find which squares black pawns attack FROM)
    Bitboard attackers = (sideToMove == WHITE)
                             ? getWhitePawnAttacks(board->epSquare)
                             : getBlackPawnAttacks(board->epSquare);

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

void genAllPseudoLegalKnightMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard knights = (sideToMove == WHITE) ? board->whiteKnights : board->blackKnights;
    Bitboard ownPieces = (sideToMove == WHITE) ? board->whitePieces : board->blackPieces;
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
        Bitboard quietMoves = attacks & ~ownPieces & ~opponentPieces;
        while (quietMoves)
        {
            Square to = bb_pop_lsb(&quietMoves);
            Move move = MAKE_MOVE(from, to);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void genAllPseudoLegalBishopMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard bishops = (sideToMove == WHITE) ? board->whiteBishops : board->blackBishops;
    Bitboard ownPieces = (sideToMove == WHITE) ? board->whitePieces : board->blackPieces;
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

        Bitboard quietMoves = attacks & ~ownPieces & ~opponentPieces;
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
    Bitboard ownPieces = (sideToMove == WHITE) ? board->whitePieces : board->blackPieces;
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

        Bitboard quietMoves = attacks & ~ownPieces & ~opponentPieces;
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
    Bitboard ownPieces = (sideToMove == WHITE) ? board->whitePieces : board->blackPieces;
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

        Bitboard quietMoves = attacks & ~ownPieces & ~opponentPieces;
        while (quietMoves)
        {
            Square to = bb_pop_lsb(&quietMoves);
            Move move = MAKE_MOVE(from, to);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void genAllPseudoLegalKingNonCastlingMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard king = (sideToMove == WHITE) ? board->whiteKing : board->blackKing;
    Bitboard ownPieces = (sideToMove == WHITE) ? board->whitePieces : board->blackPieces;
    Bitboard opponentPieces = (sideToMove == WHITE) ? board->blackPieces : board->whitePieces;
    if (king)
    {
        Square from = bb_pop_lsb(&king);
        Bitboard attacks = getKingAttacks(from, board->allPieces);

        Bitboard captures = attacks & opponentPieces;
        while (captures)
        {
            Square to = bb_pop_lsb(&captures);
            Move move = MAKE_CAPTURE(from, to);
            moveList->moves[moveList->count++] = move;
        }

        Bitboard quietMoves = attacks & ~ownPieces & ~opponentPieces;
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
        if (board->whiteCanCastleKingside)
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
        if (board->whiteCanCastleQueenside)
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
        if (board->blackCanCastleKingside)
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
        if (board->blackCanCastleQueenside)
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

void genAllPseudoLegalMoves(CBoard *board, MoveList *moveList)
{
    genAllPseudoLegalPawnMoves(board, moveList);
    genAllPseudoLegalKnightMoves(board, moveList);
    genAllPseudoLegalBishopMoves(board, moveList);
    genAllPseudoLegalRookMoves(board, moveList);
    genAllPseudoLegalQueenMoves(board, moveList);
}

void initMoveList(MoveList *moveList)
{
    moveList->count = 0;
}
