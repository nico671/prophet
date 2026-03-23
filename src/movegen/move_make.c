#include "movegen/move_make.h"
#include "movegen/move.h"
#include "core/bitboard.h"
#include "board/cboard.h"
#include "board/zobrist.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
// Helper to save undo info
static UndoInfo saveUndoInfo(CBoard *board, PieceType captured, Move move)
{
    UndoInfo undoInfo = {0};
    undoInfo.capturedPiece = captured;
    undoInfo.capturedSquare = (captured != NO_PIECE) ? getToSquare(move) : NO_SQUARE;
    undoInfo.previousEpSquare = board->epSquare;
    undoInfo.previousHalfmoveClock = board->halfmoveClock;
    undoInfo.previousCastlingRights = board->castlingRights;
    return undoInfo;
}

// Helper to update game state after move
static void updateGameState(CBoard *board, Square to, bool isCapture)
{
    // Update halfmove clock
    bool isPawnMove = bitboardIsBitSet(board->whitePawns, to) || bitboardIsBitSet(board->blackPawns, to);
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

UndoInfo makeQuietMove(CBoard *board, Move move)
{
    Square from = getFromSquare(move);
    Square to = getToSquare(move);

    // Save undo info before making changes
    UndoInfo undoInfo = saveUndoInfo(board, NO_PIECE, move);
    undoInfo.previousZobristKey = board->zobristKey;

    // Determine the piece type being moved
    PieceType movingPiece = NO_PIECE;
    movingPiece = getPieceAtSquare(board, from);

    // Update zobrist: remove piece from 'from' square
    zobristTogglePiece(&board->zobristKey, movingPiece, board->sideToMove, from);

    // Remove old EP square from hash if any
    zobristToggleEnPassant(&board->zobristKey, board, board->epSquare);

    // Save old castling rights
    uint8_t oldCastlingRights = board->castlingRights;

    // Move the piece
    movePieceOnBoard(board, from, to, board->sideToMove);

    // Update board state
    updateCastlingRights(board, from, to);
    updateOccupanciesForMove(board, from, to, board->sideToMove);

    // Update zobrist: add piece to 'to' square
    zobristTogglePiece(&board->zobristKey, movingPiece, board->sideToMove, to);

    // Update zobrist for castling rights change
    zobristToggleCastling(&board->zobristKey, oldCastlingRights, board->castlingRights);

    updateGameState(board, to, false);

    // Toggle side to move in zobrist (done after updateGameState which switches side)
    zobristToggleSide(&board->zobristKey);

    return undoInfo;
}

UndoInfo makeCaptureMove(CBoard *board, Move move)
{
    Square from = getFromSquare(move);
    Square to = getToSquare(move);

    // Determine the piece type being moved
    PieceType movingPiece = NO_PIECE;
    movingPiece = getPieceAtSquare(board, from);

    // Remove captured piece first
    PieceType captured = removeCapturedPiece(board, to, board->sideToMove);

    // Save undo info
    UndoInfo undoInfo = saveUndoInfo(board, captured, move);
    undoInfo.previousZobristKey = board->zobristKey;

    // Update zobrist: remove moving piece from 'from' square
    zobristTogglePiece(&board->zobristKey, movingPiece, board->sideToMove, from);

    // Update zobrist: remove captured piece from 'to' square
    Color capturedColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
    zobristTogglePiece(&board->zobristKey, captured, capturedColor, to);

    // Remove old EP square from hash if any
    zobristToggleEnPassant(&board->zobristKey, board, board->epSquare);

    // Save old castling rights
    uint8_t oldCastlingRights = board->castlingRights;

    // Update occupancies for capture (remove captured piece)
    updateOccupanciesForCapture(board, to, capturedColor);

    // Move the capturing piece
    movePieceOnBoard(board, from, to, board->sideToMove);

    // Update board state
    updateCastlingRights(board, from, to);
    updateOccupanciesForMove(board, from, to, board->sideToMove);

    // Update zobrist: add moving piece to 'to' square
    zobristTogglePiece(&board->zobristKey, movingPiece, board->sideToMove, to);

    // Update zobrist for castling rights change
    zobristToggleCastling(&board->zobristKey, oldCastlingRights, board->castlingRights);

    updateGameState(board, to, true);

    // Toggle side to move in zobrist
    zobristToggleSide(&board->zobristKey);

    return undoInfo;
}

UndoInfo makeDoublePawnPushMove(CBoard *board, Move move)
{
    Square from = getFromSquare(move);
    Square to = getToSquare(move);

    // Save undo info before making any changes
    UndoInfo undoInfo = saveUndoInfo(board, NO_PIECE, move);
    undoInfo.previousZobristKey = board->zobristKey;

    // Calculate en passant square BEFORE switching sides
    // EP square is the square the pawn skipped over
    Square epSquare = (board->sideToMove == WHITE) ? to - 8 : to + 8;

    // Update zobrist: remove pawn from 'from' square
    zobristTogglePiece(&board->zobristKey, PAWN, board->sideToMove, from);

    // Remove old EP square from hash if any
    zobristToggleEnPassant(&board->zobristKey, board, board->epSquare);

    // Save old castling rights
    uint8_t oldCastlingRights = board->castlingRights;

    // Move the pawn
    movePieceOnBoard(board, from, to, board->sideToMove);

    // Update board state
    updateCastlingRights(board, from, to);
    updateOccupanciesForMove(board, from, to, board->sideToMove);

    // Update zobrist: add pawn to 'to' square
    zobristTogglePiece(&board->zobristKey, PAWN, board->sideToMove, to);

    // Update zobrist for castling rights change
    zobristToggleCastling(&board->zobristKey, oldCastlingRights, board->castlingRights);

    updateGameState(board, to, false); // This switches sideToMove and clears epSquare

    // Set en passant square AFTER updateGameState (which clears it)
    board->epSquare = epSquare;

    // Add new EP square to hash
    zobristToggleEnPassant(&board->zobristKey, board, epSquare);

    // Toggle side to move in zobrist
    zobristToggleSide(&board->zobristKey);

    return undoInfo;
}

UndoInfo makeEnPassantCapture(CBoard *board, Move move)
{
    Square from = getFromSquare(move);
    Square to = getToSquare(move);

    // Determine the square of the captured pawn
    Square capturedPawnSquare = (board->sideToMove == WHITE) ? to - 8 : to + 8;

    UndoInfo undoInfo = saveUndoInfo(board, PAWN, move);
    undoInfo.previousZobristKey = board->zobristKey;

    // Update zobrist: remove capturing pawn from 'from' square
    zobristTogglePiece(&board->zobristKey, PAWN, board->sideToMove, from);

    // Update zobrist: remove captured pawn from its square
    Color capturedColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
    zobristTogglePiece(&board->zobristKey, PAWN, capturedColor, capturedPawnSquare);

    // Remove old EP square from hash
    zobristToggleEnPassant(&board->zobristKey, board, board->epSquare);

    // Save old castling rights
    uint8_t oldCastlingRights = board->castlingRights;

    // Remove the captured pawn
    removeCapturedPiece(board, capturedPawnSquare, board->sideToMove);

    // Update occupancies for capture (remove captured pawn)
    updateOccupanciesForCapture(board, capturedPawnSquare, capturedColor);

    // Move the capturing pawn
    movePieceOnBoard(board, from, to, board->sideToMove);

    // Update board state
    updateCastlingRights(board, from, to);
    updateOccupanciesForMove(board, from, to, board->sideToMove);

    // Update zobrist: add capturing pawn to 'to' square
    zobristTogglePiece(&board->zobristKey, PAWN, board->sideToMove, to);

    // Update zobrist for castling rights change
    zobristToggleCastling(&board->zobristKey, oldCastlingRights, board->castlingRights);

    updateGameState(board, to, true);

    // Toggle side to move in zobrist
    zobristToggleSide(&board->zobristKey);

    return undoInfo;
}

UndoInfo makePromotionMove(CBoard *board, Move move)
{
    Square from = getFromSquare(move);
    Square to = getToSquare(move);

    // Determine if it's a promotion capture
    bool isPromotionCapture;
    if (board->sideToMove == WHITE)
    {
        isPromotionCapture = bitboardIsBitSet(board->blackPieces, to);
    }
    else
    {
        isPromotionCapture = bitboardIsBitSet(board->whitePieces, to);
    }

    // If capture, remove the captured piece first and save its type
    PieceType captured = NO_PIECE;
    if (isPromotionCapture)
    {
        captured = removeCapturedPiece(board, to, board->sideToMove);
        // Update occupancies for capture
        Color capturedColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
        updateOccupanciesForCapture(board, to, capturedColor);
    }

    // Save undo info before making changes
    UndoInfo undoInfo = saveUndoInfo(board, captured, move);
    undoInfo.previousZobristKey = board->zobristKey;

    // Get the promotion piece type
    PieceType promotionPiece = getPromotionPieceType(move);

    // Update zobrist: remove pawn from 'from' square
    zobristTogglePiece(&board->zobristKey, PAWN, board->sideToMove, from);

    // Update zobrist: if capture, remove captured piece from 'to' square
    if (isPromotionCapture && captured != NO_PIECE)
    {
        Color capturedColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
        zobristTogglePiece(&board->zobristKey, captured, capturedColor, to);
    }

    // Remove old EP square from hash
    zobristToggleEnPassant(&board->zobristKey, board, board->epSquare);

    // Save old castling rights
    uint8_t oldCastlingRights = board->castlingRights;

    // Replace pawn with promoted piece
    removePieceFromBoard(board, from, board->sideToMove, PAWN);
    addPieceToBoard(board, to, board->sideToMove, promotionPiece);

    // Update zobrist: add promoted piece to 'to' square
    zobristTogglePiece(&board->zobristKey, promotionPiece, board->sideToMove, to);

    // Update board state
    updateCastlingRights(board, from, to);
    updateOccupanciesForPromotion(board, from, to, board->sideToMove);

    // Update zobrist for castling rights change
    zobristToggleCastling(&board->zobristKey, oldCastlingRights, board->castlingRights);

    updateGameState(board, to, isPromotionCapture);

    // Toggle side to move in zobrist
    zobristToggleSide(&board->zobristKey);

    return undoInfo;
}

UndoInfo makeCastlingMove(CBoard *board, Move move)
{
    Square to = getToSquare(move);
    Square from = getFromSquare(move);
    // Save undo info before making changes
    UndoInfo undoInfo = saveUndoInfo(board, NO_PIECE, move);
    undoInfo.previousZobristKey = board->zobristKey;

    // Remove old EP square from hash
    zobristToggleEnPassant(&board->zobristKey, board, board->epSquare);

    // Save old castling rights
    uint8_t oldCastlingRights = board->castlingRights;

    if (board->sideToMove == WHITE)
    {
        // check if kingside castle and handle accordingly (king moves e1->g1, rook moves h1->f1)
        if (from == E1 && to == G1)
        {
            // Update zobrist: remove pieces from original squares
            zobristTogglePiece(&board->zobristKey, KING, WHITE, E1);
            zobristTogglePiece(&board->zobristKey, ROOK, WHITE, H1);

            // Move king from e1 to g1 and rook from h1 to f1
            movePieceOnBoard(board, E1, G1, WHITE);
            movePieceOnBoard(board, H1, F1, WHITE);
            // Update occupancies incrementally
            updateOccupanciesForCastling(board, E1, G1, H1, F1, WHITE);

            // Update zobrist: add pieces to new squares
            zobristTogglePiece(&board->zobristKey, KING, WHITE, G1);
            zobristTogglePiece(&board->zobristKey, ROOK, WHITE, F1);
        }
        else
        { // handle queenside castle (king moves e1->c1, rook moves a1->d1)
            // Update zobrist: remove pieces from original squares
            zobristTogglePiece(&board->zobristKey, KING, WHITE, E1);
            zobristTogglePiece(&board->zobristKey, ROOK, WHITE, A1);

            // Move king from e1 to c1 and rook from a1 to d1
            movePieceOnBoard(board, E1, C1, WHITE);
            movePieceOnBoard(board, A1, D1, WHITE);
            // Update occupancies incrementally
            updateOccupanciesForCastling(board, E1, C1, A1, D1, WHITE);

            // Update zobrist: add pieces to new squares
            zobristTogglePiece(&board->zobristKey, KING, WHITE, C1);
            zobristTogglePiece(&board->zobristKey, ROOK, WHITE, D1);
        }
    }
    else
    {
        if (from == E8 && to == G8)
        {
            // Update zobrist: remove pieces from original squares
            zobristTogglePiece(&board->zobristKey, KING, BLACK, E8);
            zobristTogglePiece(&board->zobristKey, ROOK, BLACK, H8);

            // Move king from e8 to g8 and rook from h8 to f8
            movePieceOnBoard(board, E8, G8, BLACK);
            movePieceOnBoard(board, H8, F8, BLACK);
            // Update occupancies incrementally
            updateOccupanciesForCastling(board, E8, G8, H8, F8, BLACK);

            // Update zobrist: add pieces to new squares
            zobristTogglePiece(&board->zobristKey, KING, BLACK, G8);
            zobristTogglePiece(&board->zobristKey, ROOK, BLACK, F8);
        }
        else
        { // handle queenside castle (king moves e8->c8, rook moves a8->d8)
            // Update zobrist: remove pieces from original squares
            zobristTogglePiece(&board->zobristKey, KING, BLACK, E8);
            zobristTogglePiece(&board->zobristKey, ROOK, BLACK, A8);
            // Move king from e8 to c8 and rook from a8 to d8
            movePieceOnBoard(board, E8, C8, BLACK);
            movePieceOnBoard(board, A8, D8, BLACK);
            // Update occupancies incrementally
            updateOccupanciesForCastling(board, E8, C8, A8, D8, BLACK);
            // Update zobrist: add pieces to new squares
            zobristTogglePiece(&board->zobristKey, KING, BLACK, C8);
            zobristTogglePiece(&board->zobristKey, ROOK, BLACK, D8);
        }
    }
    // Update castling rights (castling removes all rights for this color)
    if (board->sideToMove == WHITE)
    {
        CLEAR_BIT(board->castlingRights, 3);
        CLEAR_BIT(board->castlingRights, 2);
    }
    else
    {
        CLEAR_BIT(board->castlingRights, 1);
        CLEAR_BIT(board->castlingRights, 0);
    }

    // Update zobrist for castling rights change
    zobristToggleCastling(&board->zobristKey, oldCastlingRights, board->castlingRights);

    // Update board state (occupancies already updated above)
    updateGameState(board, to, false);

    // Toggle side to move in zobrist
    zobristToggleSide(&board->zobristKey);

    return undoInfo;
}
// else // QUEENSIDE_CASTLE
// {
//     if (board->sideToMove == WHITE)
//     {
//         // Update zobrist: remove pieces from original squares
//         zobristTogglePiece(&board->zobristKey, KING, WHITE, E1);
//         zobristTogglePiece(&board->zobristKey, ROOK, WHITE, A1);

//         // Move king from e1 to c1 and rook from a1 to d1
//         movePieceOnBoard(board, E1, C1, WHITE);
//         movePieceOnBoard(board, A1, D1, WHITE);
//         // Update occupancies incrementally
//         updateOccupanciesForCastling(board, E1, C1, A1, D1, WHITE);

//         // Update zobrist: add pieces to new squares
//         zobristTogglePiece(&board->zobristKey, KING, WHITE, C1);
//         zobristTogglePiece(&board->zobristKey, ROOK, WHITE, D1);
//     }
//     else
//     {
//         // Update zobrist: remove pieces from original squares
//         zobristTogglePiece(&board->zobristKey, KING, BLACK, E8);
//         zobristTogglePiece(&board->zobristKey, ROOK, BLACK, A8);

//         // Move king from e8 to c8 and rook from a8 to d8
//         movePieceOnBoard(board, E8, C8, BLACK);
//         movePieceOnBoard(board, A8, D8, BLACK);
//         // Update occupancies incrementally
//         updateOccupanciesForCastling(board, E8, C8, A8, D8, BLACK);

//         // Update zobrist: add pieces to new squares
//         zobristTogglePiece(&board->zobristKey, KING, BLACK, C8);
//         zobristTogglePiece(&board->zobristKey, ROOK, BLACK, D8);
//     }
// }

UndoInfo makeMove(CBoard *board, Move move)
{
    // extract flag
    Square from = getFromSquare(move);
    Square to = getToSquare(move);
    PieceType movingPiece = getPieceAtSquare(board, from);

    if (move_is_castling(move))
    {
        if (board->sideToMove == WHITE)
        {
            if (to == G1)
            {
                return makeCastlingMove(board, move); // Kingside castle
            }
            else
            {
                return makeCastlingMove(board, move); // Queenside castle
            }
        }
        else
        {
            if (to == G8)
            {
                return makeCastlingMove(board, move); // Kingside castle
            }
            else
            {
                return makeCastlingMove(board, move); // Queenside castle
            }
        }
    }
    else if (move_is_enpassant(move))
    {
        // since ep flag is set if en passant is possible, we need to verify that the move is actually an en passant capture (should not happen if move generation is correct)
        Square capturedPawnSquare = (board->sideToMove == WHITE) ? to - 8 : to + 8;
        if (getPieceAtSquare(board, to) != NO_PIECE || getPieceAtSquare(board, capturedPawnSquare) != PAWN)
        {
            // Invalid en passant move, treat as quiet move (should not happen if move generation is correct)
            return makeQuietMove(board, move);
        }
        return makeEnPassantCapture(board, move);
    }
    else if (move_is_promotion(move))
    {
        return makePromotionMove(board, move);
    }
    else // normal move (quiet or capture)
    {
        // check for captures
        if (board->sideToMove == WHITE)
        {
            if (bitboardIsBitSet(board->blackPieces, to))
            {
                return makeCaptureMove(board, move);
            }
        }
        else
        {
            if (bitboardIsBitSet(board->whitePieces, to))
            {
                return makeCaptureMove(board, move);
            }
        }
        // Detect double pawn pushes (used to set en passant square).
        if (movingPiece == PAWN)
        {
            if (board->sideToMove == WHITE)
            {
                if ((to - from) == 16)
                {
                    return makeDoublePawnPushMove(board, move);
                }
            }
            else
            {
                if ((from - to) == 16)
                {
                    return makeDoublePawnPushMove(board, move);
                }
            }
        }
        return makeQuietMove(board, move);
    }
    return (UndoInfo){0}; // Should never reach here
}

void unmakeCastlingMove(CBoard *board, Move move)
{
    // Unmake castling

    if (board->sideToMove == WHITE)
    {
        if (getToSquare(move) == G1)
        {
            // Move king back from g1 to e1 and rook from f1 to h1
            movePieceOnBoard(board, G1, E1, WHITE);
            movePieceOnBoard(board, F1, H1, WHITE);
            updateOccupanciesForCastling(board, G1, E1, F1, H1, WHITE);
        }
        else
        {
            // Move king back from c1 to e1 and rook from d1 to a1
            movePieceOnBoard(board, C1, E1, WHITE);
            movePieceOnBoard(board, D1, A1, WHITE);
            updateOccupanciesForCastling(board, C1, E1, D1, A1, WHITE);
        }
    }
    else
    {
        if (getToSquare(move) == G8)
        {
            // Move king back from g8 to e8 and rook from f8 to h8
            movePieceOnBoard(board, G8, E8, BLACK);
            movePieceOnBoard(board, F8, H8, BLACK);
            updateOccupanciesForCastling(board, G8, E8, F8, H8, BLACK);
        }
        else
        {
            // Move king back from c8 to e8 and rook from d8 to a8
            movePieceOnBoard(board, C8, E8, BLACK);
            movePieceOnBoard(board, D8, A8, BLACK);
            updateOccupanciesForCastling(board, C8, E8, D8, A8, BLACK);
        }
    }
}
void unmakeMove(CBoard *board, Move move, UndoInfo undoInfo)
{
    Square from = getFromSquare(move);
    Square to = getToSquare(move);

    // Switch side back first (before any checks that depend on sideToMove)
    board->sideToMove = (board->sideToMove == WHITE) ? BLACK : WHITE;

    // Restore game state
    board->epSquare = undoInfo.previousEpSquare;
    board->halfmoveClock = undoInfo.previousHalfmoveClock;
    board->castlingRights = undoInfo.previousCastlingRights;
    board->zobristKey = undoInfo.previousZobristKey;

    // Decrement fullmove number if unmaking black's move
    if (board->sideToMove == BLACK)
    {
        board->fullmoveNumber--;
    }

    // Handle different move types
    if (move_is_castling(move))
    {
        unmakeCastlingMove(board, move);
    }
    else if (move_is_promotion(move))
    {
        // Unmake promotion
        PieceType promotionPiece = getPromotionPieceType(move);
        bool isPromotionCapture = (undoInfo.capturedPiece != NO_PIECE);
        updateOccupanciesForMove(board, to, from, board->sideToMove);
        // Replace promoted piece with pawn
        removePieceFromBoard(board, to, board->sideToMove, promotionPiece);
        addPieceToBoard(board, from, board->sideToMove, PAWN);

        // If promotion capture, restore captured piece
        if (isPromotionCapture && undoInfo.capturedPiece != NO_PIECE)
        {
            Color opponentColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
            addPieceToBoard(board, to, opponentColor, undoInfo.capturedPiece);
            if (opponentColor == WHITE)
                bitboardSetSquareBit(&board->whitePieces, to);
            else
                bitboardSetSquareBit(&board->blackPieces, to);
            bitboardSetSquareBit(&board->allPieces, to);
        }
    }
    else if (move_is_enpassant(move))
    {
        if (undoInfo.capturedPiece != PAWN)
        {
            // Invalid undo info for en passant, treat as quiet unmake (should not happen if move generation is correct
            printf("Warning: Invalid undo info for en passant unmake, treating as quiet unmake\n");
            // Move piece back from destination to source
            movePieceOnBoard(board, to, from, board->sideToMove);
            updateOccupanciesForMove(board, to, from, board->sideToMove);
            return;
        }
        // Unmake en passant
        // Move pawn back from destination to source
        movePieceOnBoard(board, to, from, board->sideToMove);
        updateOccupanciesForMove(board, to, from, board->sideToMove);
        // Restore captured pawn
        Square capturedPawnSquare = (board->sideToMove == WHITE) ? to - 8 : to + 8;
        Color opponentColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
        addPieceToBoard(board, capturedPawnSquare, opponentColor, PAWN);
        if (opponentColor == WHITE)
            bitboardSetSquareBit(&board->whitePieces, capturedPawnSquare);
        else
            bitboardSetSquareBit(&board->blackPieces, capturedPawnSquare);
        bitboardSetSquareBit(&board->allPieces, capturedPawnSquare);
    }
    else
    {
        // Unmake regular move (quiet, capture, double pawn push)
        // Move piece back from destination to source
        movePieceOnBoard(board, to, from, board->sideToMove);
        updateOccupanciesForMove(board, to, from, board->sideToMove);

        // If it was a capture, restore the captured piece
        if (undoInfo.capturedSquare != NO_SQUARE && undoInfo.capturedPiece != NO_PIECE)
        {
            Color opponentColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
            addPieceToBoard(board, to, opponentColor, undoInfo.capturedPiece);
            if (opponentColor == WHITE)
                bitboardSetSquareBit(&board->whitePieces, to);
            else
                bitboardSetSquareBit(&board->blackPieces, to);
            bitboardSetSquareBit(&board->allPieces, to);
        }
    }
}
