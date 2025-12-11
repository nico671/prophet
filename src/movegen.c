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
        Bitboard attacks = getKingAttacks(from);

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

// Helper function to move a piece from one square to another
static void movePieceOnBoard(CBoard *board, Square from, Square to, Color side)
{
    if (side == WHITE)
    {
        if (is_bit_set(board->whitePawns, from))
        {
            bb_clear(&board->whitePawns, from);
            bb_set(&board->whitePawns, to);
        }
        else if (is_bit_set(board->whiteKnights, from))
        {
            bb_clear(&board->whiteKnights, from);
            bb_set(&board->whiteKnights, to);
        }
        else if (is_bit_set(board->whiteBishops, from))
        {
            bb_clear(&board->whiteBishops, from);
            bb_set(&board->whiteBishops, to);
        }
        else if (is_bit_set(board->whiteRooks, from))
        {
            bb_clear(&board->whiteRooks, from);
            bb_set(&board->whiteRooks, to);
        }
        else if (is_bit_set(board->whiteQueens, from))
        {
            bb_clear(&board->whiteQueens, from);
            bb_set(&board->whiteQueens, to);
        }
        else if (is_bit_set(board->whiteKing, from))
        {
            bb_clear(&board->whiteKing, from);
            bb_set(&board->whiteKing, to);
        }
    }
    else
    {
        if (is_bit_set(board->blackPawns, from))
        {
            bb_clear(&board->blackPawns, from);
            bb_set(&board->blackPawns, to);
        }
        else if (is_bit_set(board->blackKnights, from))
        {
            bb_clear(&board->blackKnights, from);
            bb_set(&board->blackKnights, to);
        }
        else if (is_bit_set(board->blackBishops, from))
        {
            bb_clear(&board->blackBishops, from);
            bb_set(&board->blackBishops, to);
        }
        else if (is_bit_set(board->blackRooks, from))
        {
            bb_clear(&board->blackRooks, from);
            bb_set(&board->blackRooks, to);
        }
        else if (is_bit_set(board->blackQueens, from))
        {
            bb_clear(&board->blackQueens, from);
            bb_set(&board->blackQueens, to);
        }
        else if (is_bit_set(board->blackKing, from))
        {
            bb_clear(&board->blackKing, from);
            bb_set(&board->blackKing, to);
        }
    }
}

// Helper function to remove a captured piece and return its type
static PieceType removeCapturedPiece(CBoard *board, Square square, Color capturingColor)
{
    Color capturedColor = (capturingColor == WHITE) ? BLACK : WHITE;

    if (capturedColor == BLACK)
    {
        if (is_bit_set(board->blackPawns, square))
        {
            bb_clear(&board->blackPawns, square);
            return PAWN;
        }
        else if (is_bit_set(board->blackKnights, square))
        {
            bb_clear(&board->blackKnights, square);
            return KNIGHT;
        }
        else if (is_bit_set(board->blackBishops, square))
        {
            bb_clear(&board->blackBishops, square);
            return BISHOP;
        }
        else if (is_bit_set(board->blackRooks, square))
        {
            bb_clear(&board->blackRooks, square);
            return ROOK;
        }
        else if (is_bit_set(board->blackQueens, square))
        {
            bb_clear(&board->blackQueens, square);
            return QUEEN;
        }
        else if (is_bit_set(board->blackKing, square))
        {
            bb_clear(&board->blackKing, square);
            return KING;
        }
    }
    else
    {
        if (is_bit_set(board->whitePawns, square))
        {
            bb_clear(&board->whitePawns, square);
            return PAWN;
        }
        else if (is_bit_set(board->whiteKnights, square))
        {
            bb_clear(&board->whiteKnights, square);
            return KNIGHT;
        }
        else if (is_bit_set(board->whiteBishops, square))
        {
            bb_clear(&board->whiteBishops, square);
            return BISHOP;
        }
        else if (is_bit_set(board->whiteRooks, square))
        {
            bb_clear(&board->whiteRooks, square);
            return ROOK;
        }
        else if (is_bit_set(board->whiteQueens, square))
        {
            bb_clear(&board->whiteQueens, square);
            return QUEEN;
        }
        else if (is_bit_set(board->whiteKing, square))
        {
            bb_clear(&board->whiteKing, square);
            return KING;
        }
    }

    return NO_PIECE;
}

// Helper to update castling rights
static void updateCastlingRights(CBoard *board, Square from, Square to)
{
    // If king moved, lose all castling
    if (is_bit_set(board->whiteKing, to))
    {
        board->whiteCanCastleKingside = false;
        board->whiteCanCastleQueenside = false;
    }
    else if (is_bit_set(board->blackKing, to))
    {
        board->blackCanCastleKingside = false;
        board->blackCanCastleQueenside = false;
    }

    // If rook moved from corner, lose that side's castling
    if (from == H1)
        board->whiteCanCastleKingside = false;
    if (from == A1)
        board->whiteCanCastleQueenside = false;
    if (from == H8)
        board->blackCanCastleKingside = false;
    if (from == A8)
        board->blackCanCastleQueenside = false;

    // If rook was captured on corner square, lose that side's castling
    if (to == H1)
        board->whiteCanCastleKingside = false;
    if (to == A1)
        board->whiteCanCastleQueenside = false;
    if (to == H8)
        board->blackCanCastleKingside = false;
    if (to == A8)
        board->blackCanCastleQueenside = false;
}

// Helper to save undo info
static UndoInfo saveUndoInfo(CBoard *board, PieceType captured)
{
    UndoInfo undoInfo = {0};
    undoInfo.capturedPiece = captured;
    undoInfo.previousEpSquare = board->epSquare;
    undoInfo.previousHalfmoveClock = board->halfmoveClock;
    undoInfo.prevWhiteCastleKingside = board->whiteCanCastleKingside;
    undoInfo.prevWhiteCastleQueenside = board->whiteCanCastleQueenside;
    undoInfo.prevBlackCastleKingside = board->blackCanCastleKingside;
    undoInfo.prevBlackCastleQueenside = board->blackCanCastleQueenside;
    return undoInfo;
}

// Helper to update game state after move
static void updateGameState(CBoard *board, Square to, bool isCapture)
{
    // Update halfmove clock
    bool isPawnMove = is_bit_set(board->whitePawns, to) || is_bit_set(board->blackPawns, to);
    if (isPawnMove || isCapture)
    {
        board->halfmoveClock = 0;
    }
    else
    {
        board->halfmoveClock++;
    }

    // Clear en passant square
    board->epSquare = NO_SQUARE;

    // Switch sides
    board->sideToMove = (board->sideToMove == WHITE) ? BLACK : WHITE;

    // Increment fullmove after black moves
    if (board->sideToMove == WHITE)
    {
        board->fullmoveNumber++;
    }
}

// Now your move functions become MUCH shorter:
UndoInfo makeQuietMove(CBoard *board, Move move)
{
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);

    // Save undo info before making changes
    UndoInfo undoInfo = saveUndoInfo(board, NO_PIECE);

    // Move the piece
    movePieceOnBoard(board, from, to, board->sideToMove);

    // Update board state
    updateCastlingRights(board, from, to);
    recomputeOccupancies(board);
    updateGameState(board, to, false);

    return undoInfo;
}

UndoInfo makeCaptureMove(CBoard *board, Move move)
{
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);

    // Remove captured piece first
    PieceType captured = removeCapturedPiece(board, to, board->sideToMove);

    // Save undo info
    UndoInfo undoInfo = saveUndoInfo(board, captured);

    // Move the capturing piece
    movePieceOnBoard(board, from, to, board->sideToMove);

    // Update board state
    updateCastlingRights(board, from, to);
    recomputeOccupancies(board);
    updateGameState(board, to, true);

    return undoInfo;
}

UndoInfo makeDoublePawnPushMove(CBoard *board, Move move)
{
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);

    // Save undo info before making any changes
    UndoInfo undoInfo = saveUndoInfo(board, NO_PIECE);

    // Move the pawn
    movePieceOnBoard(board, from, to, board->sideToMove);

    // Update board state
    updateCastlingRights(board, from, to);
    recomputeOccupancies(board);
    updateGameState(board, to, false);

    // Set en passant square (after updateGameState which clears it)
    // Note: sideToMove has been switched by updateGameState, so we use the opposite logic
    board->epSquare = (board->sideToMove == BLACK) ? to - 8 : to + 8;

    return undoInfo;
}

UndoInfo makeEnPassantMove(CBoard *board, Move move)
{
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);

    // Determine the square of the captured pawn
    Square capturedPawnSquare = (board->sideToMove == WHITE) ? to - 8 : to + 8;

    UndoInfo undoInfo = saveUndoInfo(board, PAWN);

    // Remove the captured pawn
    removeCapturedPiece(board, capturedPawnSquare, board->sideToMove);

    // Move the capturing pawn
    movePieceOnBoard(board, from, to, board->sideToMove);

    // Update board state
    updateCastlingRights(board, from, to);
    recomputeOccupancies(board);
    updateGameState(board, to, true);

    return undoInfo;
}

UndoInfo makePromotionMove(CBoard *board, Move move)
{
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);
    MoveFlag flag = MOVE_FLAG(move);

    // Determine if it's a promotion capture
    bool isPromotionCapture = (flag >= KNIGHT_PROMO_CAPTURE && flag <= QUEEN_PROMO_CAPTURE);

    // If capture, remove the captured piece first and save its type
    PieceType captured = NO_PIECE;
    if (isPromotionCapture)
    {
        captured = removeCapturedPiece(board, to, board->sideToMove);
    }

    // Save undo info before making changes
    UndoInfo undoInfo = saveUndoInfo(board, captured);

    // Remove pawn from source square
    if (board->sideToMove == WHITE)
    {
        bb_clear(&board->whitePawns, from);
    }
    else
    {
        bb_clear(&board->blackPawns, from);
    }

    // Get the promotion piece type
    PieceType promotionPiece = getPromotionPiece(move);

    // Place promoted piece on destination square
    if (board->sideToMove == WHITE)
    {
        switch (promotionPiece)
        {
        case KNIGHT:
            bb_set(&board->whiteKnights, to);
            break;
        case BISHOP:
            bb_set(&board->whiteBishops, to);
            break;
        case ROOK:
            bb_set(&board->whiteRooks, to);
            break;
        case QUEEN:
            bb_set(&board->whiteQueens, to);
            break;
        default:
            break;
        }
    }
    else
    {
        switch (promotionPiece)
        {
        case KNIGHT:
            bb_set(&board->blackKnights, to);
            break;
        case BISHOP:
            bb_set(&board->blackBishops, to);
            break;
        case ROOK:
            bb_set(&board->blackRooks, to);
            break;
        case QUEEN:
            bb_set(&board->blackQueens, to);
            break;
        default:
            break;
        }
    }

    // Update board state
    updateCastlingRights(board, from, to);
    recomputeOccupancies(board);
    updateGameState(board, to, isPromotionCapture);

    return undoInfo;
}

UndoInfo makeCastlingMove(CBoard *board, Move move)
{
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);
    MoveFlag flag = MOVE_FLAG(move);

    // Save undo info before making changes
    UndoInfo undoInfo = saveUndoInfo(board, NO_PIECE);

    if (flag == KINGSIDE_CASTLE)
    {
        if (board->sideToMove == WHITE)
        {
            // Move king from e1 to g1
            bb_clear(&board->whiteKing, E1);
            bb_set(&board->whiteKing, G1);
            // Move rook from h1 to f1
            bb_clear(&board->whiteRooks, H1);
            bb_set(&board->whiteRooks, F1);
        }
        else
        {
            // Move king from e8 to g8
            bb_clear(&board->blackKing, E8);
            bb_set(&board->blackKing, G8);
            // Move rook from h8 to f8
            bb_clear(&board->blackRooks, H8);
            bb_set(&board->blackRooks, F8);
        }
    }
    else // QUEENSIDE_CASTLE
    {
        if (board->sideToMove == WHITE)
        {
            // Move king from e1 to c1
            bb_clear(&board->whiteKing, E1);
            bb_set(&board->whiteKing, C1);
            // Move rook from a1 to d1
            bb_clear(&board->whiteRooks, A1);
            bb_set(&board->whiteRooks, D1);
        }
        else
        {
            // Move king from e8 to c8
            bb_clear(&board->blackKing, E8);
            bb_set(&board->blackKing, C8);
            // Move rook from a8 to d8
            bb_clear(&board->blackRooks, A8);
            bb_set(&board->blackRooks, D8);
        }
    }

    // Update castling rights (castling removes all rights for this color)
    if (board->sideToMove == WHITE)
    {
        board->whiteCanCastleKingside = false;
        board->whiteCanCastleQueenside = false;
    }
    else
    {
        board->blackCanCastleKingside = false;
        board->blackCanCastleQueenside = false;
    }

    // Update board state
    recomputeOccupancies(board);
    updateGameState(board, to, false);

    return undoInfo;
}

UndoInfo makeMove(CBoard *board, Move move)
{
    // extract flag
    MoveFlag flag = MOVE_FLAG(move);

    if (flag == QUIET)
    {
        return makeQuietMove(board, move);
    }
    else if (flag == DOUBLE_PAWN_PUSH)
    {
        return makeDoublePawnPushMove(board, move);
    }
    else if (flag == KINGSIDE_CASTLE || flag == QUEENSIDE_CASTLE)
    {
        return makeCastlingMove(board, move);
    }
    else if (flag == CAPTURE)
    {
        return makeCaptureMove(board, move);
    }
    else if (flag == EP_CAPTURE)
    {
        return makeEnPassantMove(board, move);
    }
    else if (flag >= KNIGHT_PROMO_QUIET && flag <= QUEEN_PROMO_CAPTURE)
    {
        return makePromotionMove(board, move);
    }
    else
    {
        // Unknown move flag - return empty undo info
        UndoInfo undoInfo = {0};
        return undoInfo;
    }
}

void unmakeMove(CBoard *board, Move move, UndoInfo undoInfo)
{
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);
    MoveFlag flag = MOVE_FLAG(move);

    // Switch side back first (before any checks that depend on sideToMove)
    board->sideToMove = (board->sideToMove == WHITE) ? BLACK : WHITE;

    // Restore game state
    board->epSquare = undoInfo.previousEpSquare;
    board->halfmoveClock = undoInfo.previousHalfmoveClock;
    board->whiteCanCastleKingside = undoInfo.prevWhiteCastleKingside;
    board->whiteCanCastleQueenside = undoInfo.prevWhiteCastleQueenside;
    board->blackCanCastleKingside = undoInfo.prevBlackCastleKingside;
    board->blackCanCastleQueenside = undoInfo.prevBlackCastleQueenside;

    // Decrement fullmove number if unmaking black's move
    if (board->sideToMove == BLACK)
    {
        board->fullmoveNumber--;
    }

    // Handle different move types
    if (flag == KINGSIDE_CASTLE || flag == QUEENSIDE_CASTLE)
    {
        // Unmake castling
        if (flag == KINGSIDE_CASTLE)
        {
            if (board->sideToMove == WHITE)
            {
                // Move king back from g1 to e1
                bb_clear(&board->whiteKing, G1);
                bb_set(&board->whiteKing, E1);
                // Move rook back from f1 to h1
                bb_clear(&board->whiteRooks, F1);
                bb_set(&board->whiteRooks, H1);
            }
            else
            {
                // Move king back from g8 to e8
                bb_clear(&board->blackKing, G8);
                bb_set(&board->blackKing, E8);
                // Move rook back from f8 to h8
                bb_clear(&board->blackRooks, F8);
                bb_set(&board->blackRooks, H8);
            }
        }
        else // QUEENSIDE_CASTLE
        {
            if (board->sideToMove == WHITE)
            {
                // Move king back from c1 to e1
                bb_clear(&board->whiteKing, C1);
                bb_set(&board->whiteKing, E1);
                // Move rook back from d1 to a1
                bb_clear(&board->whiteRooks, D1);
                bb_set(&board->whiteRooks, A1);
            }
            else
            {
                // Move king back from c8 to e8
                bb_clear(&board->blackKing, C8);
                bb_set(&board->blackKing, E8);
                // Move rook back from d8 to a8
                bb_clear(&board->blackRooks, D8);
                bb_set(&board->blackRooks, A8);
            }
        }
    }
    else if (flag >= KNIGHT_PROMO_QUIET && flag <= QUEEN_PROMO_CAPTURE)
    {
        // Unmake promotion
        PieceType promotionPiece = getPromotionPiece(move);
        bool isPromotionCapture = (flag >= KNIGHT_PROMO_CAPTURE && flag <= QUEEN_PROMO_CAPTURE);

        // Remove promoted piece from destination
        if (board->sideToMove == WHITE)
        {
            switch (promotionPiece)
            {
            case KNIGHT:
                bb_clear(&board->whiteKnights, to);
                break;
            case BISHOP:
                bb_clear(&board->whiteBishops, to);
                break;
            case ROOK:
                bb_clear(&board->whiteRooks, to);
                break;
            case QUEEN:
                bb_clear(&board->whiteQueens, to);
                break;
            default:
                break;
            }
        }
        else
        {
            switch (promotionPiece)
            {
            case KNIGHT:
                bb_clear(&board->blackKnights, to);
                break;
            case BISHOP:
                bb_clear(&board->blackBishops, to);
                break;
            case ROOK:
                bb_clear(&board->blackRooks, to);
                break;
            case QUEEN:
                bb_clear(&board->blackQueens, to);
                break;
            default:
                break;
            }
        }

        // Restore pawn to source square
        if (board->sideToMove == WHITE)
        {
            bb_set(&board->whitePawns, from);
        }
        else
        {
            bb_set(&board->blackPawns, from);
        }

        // If promotion capture, restore captured piece
        if (isPromotionCapture && undoInfo.capturedPiece != NO_PIECE)
        {
            Color opponentColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
            if (opponentColor == BLACK)
            {
                switch (undoInfo.capturedPiece)
                {
                case PAWN:
                    bb_set(&board->blackPawns, to);
                    break;
                case KNIGHT:
                    bb_set(&board->blackKnights, to);
                    break;
                case BISHOP:
                    bb_set(&board->blackBishops, to);
                    break;
                case ROOK:
                    bb_set(&board->blackRooks, to);
                    break;
                case QUEEN:
                    bb_set(&board->blackQueens, to);
                    break;
                default:
                    break;
                }
            }
            else
            {
                switch (undoInfo.capturedPiece)
                {
                case PAWN:
                    bb_set(&board->whitePawns, to);
                    break;
                case KNIGHT:
                    bb_set(&board->whiteKnights, to);
                    break;
                case BISHOP:
                    bb_set(&board->whiteBishops, to);
                    break;
                case ROOK:
                    bb_set(&board->whiteRooks, to);
                    break;
                case QUEEN:
                    bb_set(&board->whiteQueens, to);
                    break;
                default:
                    break;
                }
            }
        }
    }
    else if (flag == EP_CAPTURE)
    {
        // Unmake en passant
        // Move pawn back from destination to source
        movePieceOnBoard(board, to, from, board->sideToMove);

        // Restore captured pawn
        Square capturedPawnSquare = (board->sideToMove == WHITE) ? to - 8 : to + 8;
        Color opponentColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
        if (opponentColor == BLACK)
        {
            bb_set(&board->blackPawns, capturedPawnSquare);
        }
        else
        {
            bb_set(&board->whitePawns, capturedPawnSquare);
        }
    }
    else
    {
        // Unmake regular move (quiet, capture, double pawn push)
        // Move piece back from destination to source
        movePieceOnBoard(board, to, from, board->sideToMove);

        // If it was a capture, restore the captured piece
        if (flag == CAPTURE && undoInfo.capturedPiece != NO_PIECE)
        {
            Color opponentColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
            if (opponentColor == BLACK)
            {
                switch (undoInfo.capturedPiece)
                {
                case PAWN:
                    bb_set(&board->blackPawns, to);
                    break;
                case KNIGHT:
                    bb_set(&board->blackKnights, to);
                    break;
                case BISHOP:
                    bb_set(&board->blackBishops, to);
                    break;
                case ROOK:
                    bb_set(&board->blackRooks, to);
                    break;
                case QUEEN:
                    bb_set(&board->blackQueens, to);
                    break;
                case KING:
                    bb_set(&board->blackKing, to);
                    break;
                default:
                    break;
                }
            }
            else
            {
                switch (undoInfo.capturedPiece)
                {
                case PAWN:
                    bb_set(&board->whitePawns, to);
                    break;
                case KNIGHT:
                    bb_set(&board->whiteKnights, to);
                    break;
                case BISHOP:
                    bb_set(&board->whiteBishops, to);
                    break;
                case ROOK:
                    bb_set(&board->whiteRooks, to);
                    break;
                case QUEEN:
                    bb_set(&board->whiteQueens, to);
                    break;
                case KING:
                    bb_set(&board->whiteKing, to);
                    break;
                default:
                    break;
                }
            }
        }
    }

    // Recompute occupancy bitboards
    recomputeOccupancies(board);
}