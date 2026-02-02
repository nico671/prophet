#include "movegen/move_make.h"
#include "core/bitboard.h"
#include "board/cboard.h"
#include "board/zobrist.h"
#include <stdbool.h>

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

// The recomputeOccupancies() function is still available in cboard.c for full recomputation, but testing seems to hold with incremental updates.

// Incremental occupancy update helpers
// Update occupancies when moving a piece from one square to another
static inline void updateOccupanciesForMove(CBoard *board, Square from, Square to, Color color)
{
    if (color == WHITE)
    {
        bb_clear(&board->whitePieces, from);
        bb_set(&board->whitePieces, to);
    }
    else
    {
        bb_clear(&board->blackPieces, from);
        bb_set(&board->blackPieces, to);
    }
    bb_clear(&board->allPieces, from);
    bb_set(&board->allPieces, to);
}

// Update occupancies when capturing a piece
static inline void updateOccupanciesForCapture(CBoard *board, Square square, Color capturedColor)
{
    if (capturedColor == WHITE)
    {
        bb_clear(&board->whitePieces, square);
    }
    else
    {
        bb_clear(&board->blackPieces, square);
    }
    bb_clear(&board->allPieces, square);
}

// Update occupancies for promotion (pawn removed from 'from', promoted piece added to 'to')
static inline void updateOccupanciesForPromotion(CBoard *board, Square from, Square to, Color color)
{
    // Same as a regular move - color occupancy changes from 'from' to 'to'
    // Piece type changes but color stays the same
    updateOccupanciesForMove(board, from, to, color);
}

// Update occupancies for castling (king and rook both move)
static inline void updateOccupanciesForCastling(CBoard *board, Square kingFrom, Square kingTo,
                                                Square rookFrom, Square rookTo, Color color)
{
    if (color == WHITE)
    {
        bb_clear(&board->whitePieces, kingFrom);
        bb_clear(&board->whitePieces, rookFrom);
        bb_set(&board->whitePieces, kingTo);
        bb_set(&board->whitePieces, rookTo);
    }
    else
    {
        bb_clear(&board->blackPieces, kingFrom);
        bb_clear(&board->blackPieces, rookFrom);
        bb_set(&board->blackPieces, kingTo);
        bb_set(&board->blackPieces, rookTo);
    }
    bb_clear(&board->allPieces, kingFrom);
    bb_clear(&board->allPieces, rookFrom);
    bb_set(&board->allPieces, kingTo);
    bb_set(&board->allPieces, rookTo);
}

// Helper to update castling rights
static void updateCastlingRights(CBoard *board, Square from, Square to)
{
    // If king moved, lose all castling
    if (is_bit_set(board->whiteKing, to))
    {
        CLEAR_BIT(board->castlingRights, 3);
        CLEAR_BIT(board->castlingRights, 2);
        // board->whiteCanCastleQueenside = false;
    }
    else if (is_bit_set(board->blackKing, to))
    {
        CLEAR_BIT(board->castlingRights, 1);
        CLEAR_BIT(board->castlingRights, 0);
        // board->blackCanCastleKingside = false;
        // board->blackCanCastleQueenside = false;
    }

    // If rook moved from corner, lose that side's castling
    if (from == H1)
        CLEAR_BIT(board->castlingRights, 3);
    if (from == A1)
        CLEAR_BIT(board->castlingRights, 2);
    if (from == H8)
        CLEAR_BIT(board->castlingRights, 1);
    if (from == A8)
        CLEAR_BIT(board->castlingRights, 0);

    // If rook was captured on corner square, lose that side's castling
    if (to == H1)
        CLEAR_BIT(board->castlingRights, 3);
    if (to == A1)
        CLEAR_BIT(board->castlingRights, 2);
    if (to == H8)
        CLEAR_BIT(board->castlingRights, 1);
    if (to == A8)
        CLEAR_BIT(board->castlingRights, 0);
}

// Helper to save undo info
static UndoInfo saveUndoInfo(CBoard *board, PieceType captured)
{
    UndoInfo undoInfo = {0};
    undoInfo.capturedPiece = captured;
    undoInfo.previousEpSquare = board->epSquare;
    undoInfo.previousHalfmoveClock = board->halfmoveClock;
    undoInfo.previousCastlingRights = board->castlingRights;
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

UndoInfo makeQuietMove(CBoard *board, Move move)
{
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);

    // Save undo info before making changes
    UndoInfo undoInfo = saveUndoInfo(board, NO_PIECE);
    undoInfo.previousZobristKey = board->zobristKey;

    // Determine the piece type being moved
    PieceType movingPiece = NO_PIECE;
    if (is_bit_set(board->whitePawns, from) || is_bit_set(board->blackPawns, from))
        movingPiece = PAWN;
    else if (is_bit_set(board->whiteKnights, from) || is_bit_set(board->blackKnights, from))
        movingPiece = KNIGHT;
    else if (is_bit_set(board->whiteBishops, from) || is_bit_set(board->blackBishops, from))
        movingPiece = BISHOP;
    else if (is_bit_set(board->whiteRooks, from) || is_bit_set(board->blackRooks, from))
        movingPiece = ROOK;
    else if (is_bit_set(board->whiteQueens, from) || is_bit_set(board->blackQueens, from))
        movingPiece = QUEEN;
    else if (is_bit_set(board->whiteKing, from) || is_bit_set(board->blackKing, from))
        movingPiece = KING;

    // Update zobrist: remove piece from 'from' square
    zobristTogglePiece(&board->zobristKey, movingPiece, board->sideToMove, from);

    // Remove old EP square from hash if any
    zobristToggleEnPassant(&board->zobristKey, board->epSquare);

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
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);

    // Determine the piece type being moved
    PieceType movingPiece = NO_PIECE;
    if (is_bit_set(board->whitePawns, from) || is_bit_set(board->blackPawns, from))
        movingPiece = PAWN;
    else if (is_bit_set(board->whiteKnights, from) || is_bit_set(board->blackKnights, from))
        movingPiece = KNIGHT;
    else if (is_bit_set(board->whiteBishops, from) || is_bit_set(board->blackBishops, from))
        movingPiece = BISHOP;
    else if (is_bit_set(board->whiteRooks, from) || is_bit_set(board->blackRooks, from))
        movingPiece = ROOK;
    else if (is_bit_set(board->whiteQueens, from) || is_bit_set(board->blackQueens, from))
        movingPiece = QUEEN;
    else if (is_bit_set(board->whiteKing, from) || is_bit_set(board->blackKing, from))
        movingPiece = KING;

    // Remove captured piece first
    PieceType captured = removeCapturedPiece(board, to, board->sideToMove);

    // Save undo info
    UndoInfo undoInfo = saveUndoInfo(board, captured);
    undoInfo.previousZobristKey = board->zobristKey;

    // Update zobrist: remove moving piece from 'from' square
    zobristTogglePiece(&board->zobristKey, movingPiece, board->sideToMove, from);

    // Update zobrist: remove captured piece from 'to' square
    Color capturedColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
    zobristTogglePiece(&board->zobristKey, captured, capturedColor, to);

    // Remove old EP square from hash if any
    zobristToggleEnPassant(&board->zobristKey, board->epSquare);

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
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);

    // Save undo info before making any changes
    UndoInfo undoInfo = saveUndoInfo(board, NO_PIECE);
    undoInfo.previousZobristKey = board->zobristKey;

    // Calculate en passant square BEFORE switching sides
    // EP square is the square the pawn skipped over
    Square epSquare = (board->sideToMove == WHITE) ? to - 8 : to + 8;

    // Update zobrist: remove pawn from 'from' square
    zobristTogglePiece(&board->zobristKey, PAWN, board->sideToMove, from);

    // Remove old EP square from hash if any
    zobristToggleEnPassant(&board->zobristKey, board->epSquare);

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
    zobristToggleEnPassant(&board->zobristKey, epSquare);

    // Toggle side to move in zobrist
    zobristToggleSide(&board->zobristKey);

    return undoInfo;
}

UndoInfo makeEnPassantMove(CBoard *board, Move move)
{
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);

    // Determine the square of the captured pawn
    Square capturedPawnSquare = (board->sideToMove == WHITE) ? to - 8 : to + 8;

    UndoInfo undoInfo = saveUndoInfo(board, PAWN);
    undoInfo.previousZobristKey = board->zobristKey;

    // Update zobrist: remove capturing pawn from 'from' square
    zobristTogglePiece(&board->zobristKey, PAWN, board->sideToMove, from);

    // Update zobrist: remove captured pawn from its square
    Color capturedColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
    zobristTogglePiece(&board->zobristKey, PAWN, capturedColor, capturedPawnSquare);

    // Remove old EP square from hash
    zobristToggleEnPassant(&board->zobristKey, board->epSquare);

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
        // Update occupancies for capture
        Color capturedColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
        updateOccupanciesForCapture(board, to, capturedColor);
    }

    // Save undo info before making changes
    UndoInfo undoInfo = saveUndoInfo(board, captured);
    undoInfo.previousZobristKey = board->zobristKey;

    // Get the promotion piece type
    PieceType promotionPiece = move_promotion_piece(move);

    // Update zobrist: remove pawn from 'from' square
    zobristTogglePiece(&board->zobristKey, PAWN, board->sideToMove, from);

    // Update zobrist: if capture, remove captured piece from 'to' square
    if (isPromotionCapture && captured != NO_PIECE)
    {
        Color capturedColor = (board->sideToMove == WHITE) ? BLACK : WHITE;
        zobristTogglePiece(&board->zobristKey, captured, capturedColor, to);
    }

    // Remove old EP square from hash
    zobristToggleEnPassant(&board->zobristKey, board->epSquare);

    // Save old castling rights
    uint8_t oldCastlingRights = board->castlingRights;

    // Remove pawn from source square
    if (board->sideToMove == WHITE)
    {
        bb_clear(&board->whitePawns, from);
    }
    else
    {
        bb_clear(&board->blackPawns, from);
    }

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
    Square to = TO_SQ(move);
    MoveFlag flag = MOVE_FLAG(move);

    // Save undo info before making changes
    UndoInfo undoInfo = saveUndoInfo(board, NO_PIECE);
    undoInfo.previousZobristKey = board->zobristKey;

    // Remove old EP square from hash
    zobristToggleEnPassant(&board->zobristKey, board->epSquare);

    // Save old castling rights
    uint8_t oldCastlingRights = board->castlingRights;

    if (flag == KINGSIDE_CASTLE)
    {
        if (board->sideToMove == WHITE)
        {
            // Update zobrist: remove pieces from original squares
            zobristTogglePiece(&board->zobristKey, KING, WHITE, E1);
            zobristTogglePiece(&board->zobristKey, ROOK, WHITE, H1);

            // Move king from e1 to g1
            bb_clear(&board->whiteKing, E1);
            bb_set(&board->whiteKing, G1);
            // Move rook from h1 to f1
            bb_clear(&board->whiteRooks, H1);
            bb_set(&board->whiteRooks, F1);
            // Update occupancies incrementally
            updateOccupanciesForCastling(board, E1, G1, H1, F1, WHITE);

            // Update zobrist: add pieces to new squares
            zobristTogglePiece(&board->zobristKey, KING, WHITE, G1);
            zobristTogglePiece(&board->zobristKey, ROOK, WHITE, F1);
        }
        else
        {
            // Update zobrist: remove pieces from original squares
            zobristTogglePiece(&board->zobristKey, KING, BLACK, E8);
            zobristTogglePiece(&board->zobristKey, ROOK, BLACK, H8);

            // Move king from e8 to g8
            bb_clear(&board->blackKing, E8);
            bb_set(&board->blackKing, G8);
            // Move rook from h8 to f8
            bb_clear(&board->blackRooks, H8);
            bb_set(&board->blackRooks, F8);
            // Update occupancies incrementally
            updateOccupanciesForCastling(board, E8, G8, H8, F8, BLACK);

            // Update zobrist: add pieces to new squares
            zobristTogglePiece(&board->zobristKey, KING, BLACK, G8);
            zobristTogglePiece(&board->zobristKey, ROOK, BLACK, F8);
        }
    }
    else // QUEENSIDE_CASTLE
    {
        if (board->sideToMove == WHITE)
        {
            // Update zobrist: remove pieces from original squares
            zobristTogglePiece(&board->zobristKey, KING, WHITE, E1);
            zobristTogglePiece(&board->zobristKey, ROOK, WHITE, A1);

            // Move king from e1 to c1
            bb_clear(&board->whiteKing, E1);
            bb_set(&board->whiteKing, C1);
            // Move rook from a1 to d1
            bb_clear(&board->whiteRooks, A1);
            bb_set(&board->whiteRooks, D1);
            // Update occupancies incrementally
            updateOccupanciesForCastling(board, E1, C1, A1, D1, WHITE);

            // Update zobrist: add pieces to new squares
            zobristTogglePiece(&board->zobristKey, KING, WHITE, C1);
            zobristTogglePiece(&board->zobristKey, ROOK, WHITE, D1);
        }
        else
        {
            // Update zobrist: remove pieces from original squares
            zobristTogglePiece(&board->zobristKey, KING, BLACK, E8);
            zobristTogglePiece(&board->zobristKey, ROOK, BLACK, A8);

            // Move king from e8 to c8
            bb_clear(&board->blackKing, E8);
            bb_set(&board->blackKing, C8);
            // Move rook from a8 to d8
            bb_clear(&board->blackRooks, A8);
            bb_set(&board->blackRooks, D8);
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
    board->castlingRights = undoInfo.previousCastlingRights;
    board->zobristKey = undoInfo.previousZobristKey;

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
        PieceType promotionPiece = move_promotion_piece(move);
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
