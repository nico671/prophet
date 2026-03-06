#include "movegen/move_make.h"
#include "core/bitboard.h"
#include "board/cboard.h"
#include "board/zobrist.h"
#include <stdbool.h>
#include <stddef.h>

static inline Bitboard *pieceBitboard(CBoard *board, Color color, PieceType piece)
{
    if (color == WHITE)
    {
        switch (piece)
        {
        case PAWN:
            return &board->whitePawns;
        case KNIGHT:
            return &board->whiteKnights;
        case BISHOP:
            return &board->whiteBishops;
        case ROOK:
            return &board->whiteRooks;
        case QUEEN:
            return &board->whiteQueens;
        case KING:
            return &board->whiteKing;
        default:
            return NULL;
        }
    }
    else
    {
        switch (piece)
        {
        case PAWN:
            return &board->blackPawns;
        case KNIGHT:
            return &board->blackKnights;
        case BISHOP:
            return &board->blackBishops;
        case ROOK:
            return &board->blackRooks;
        case QUEEN:
            return &board->blackQueens;
        case KING:
            return &board->blackKing;
        default:
            return NULL;
        }
    }
}

static inline void addPieceToBoard(CBoard *board, Square square, Color color, PieceType piece)
{
    Bitboard *bb = pieceBitboard(board, color, piece);
    if (!bb)
    {
        return;
    }

    bitboardSetSquareBit(bb, square);
    board->pieceAtSquare[square] = piece;
}

static inline void removePieceFromBoard(CBoard *board, Square square, Color color, PieceType piece)
{
    Bitboard *bb = pieceBitboard(board, color, piece);
    if (!bb)
    {
        return;
    }

    bitboardClearSquareBit(bb, square);
    board->pieceAtSquare[square] = NO_PIECE;
}

// Helper function to move a piece from one square to another
static void movePieceOnBoard(CBoard *board, Square from, Square to, Color side)
{
    PieceType movingPiece = getPieceAtSquare(board, from);
    if (movingPiece == NO_PIECE)
    {
        return;
    }

    removePieceFromBoard(board, from, side, movingPiece);
    addPieceToBoard(board, to, side, movingPiece);
}

// Helper function to remove a captured piece and return its type
static PieceType removeCapturedPiece(CBoard *board, Square square, Color capturingColor)
{
    Color capturedColor = (capturingColor == WHITE) ? BLACK : WHITE;

    PieceType capturedPiece = getPieceAtSquare(board, square);
    if (capturedPiece == NO_PIECE)
    {
        return NO_PIECE;
    }

    removePieceFromBoard(board, square, capturedColor, capturedPiece);
    return capturedPiece;
}

// The recomputeOccupancies() function is still available in cboard.c for full recomputation, but testing seems to hold with incremental updates.

// Incremental occupancy update helpers
// Update occupancies when moving a piece from one square to another
static inline void updateOccupanciesForMove(CBoard *board, Square from, Square to, Color color)
{
    if (color == WHITE)
    {
        bitboardClearSquareBit(&board->whitePieces, from);
        bitboardSetSquareBit(&board->whitePieces, to);
    }
    else
    {
        bitboardClearSquareBit(&board->blackPieces, from);
        bitboardSetSquareBit(&board->blackPieces, to);
    }
    bitboardClearSquareBit(&board->allPieces, from);
    bitboardSetSquareBit(&board->allPieces, to);
}

// Update occupancies when capturing a piece
static inline void updateOccupanciesForCapture(CBoard *board, Square square, Color capturedColor)
{
    if (capturedColor == WHITE)
    {
        bitboardClearSquareBit(&board->whitePieces, square);
    }
    else
    {
        bitboardClearSquareBit(&board->blackPieces, square);
    }
    bitboardClearSquareBit(&board->allPieces, square);
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
        bitboardClearSquareBit(&board->whitePieces, kingFrom);
        bitboardClearSquareBit(&board->whitePieces, rookFrom);
        bitboardSetSquareBit(&board->whitePieces, kingTo);
        bitboardSetSquareBit(&board->whitePieces, rookTo);
    }
    else
    {
        bitboardClearSquareBit(&board->blackPieces, kingFrom);
        bitboardClearSquareBit(&board->blackPieces, rookFrom);
        bitboardSetSquareBit(&board->blackPieces, kingTo);
        bitboardSetSquareBit(&board->blackPieces, rookTo);
    }
    bitboardClearSquareBit(&board->allPieces, kingFrom);
    bitboardClearSquareBit(&board->allPieces, rookFrom);
    bitboardSetSquareBit(&board->allPieces, kingTo);
    bitboardSetSquareBit(&board->allPieces, rookTo);
}

// Helper to update castling rights
static void updateCastlingRights(CBoard *board, Square from, Square to)
{
    // If king moved, lose all castling
    if (bitboardIsBitSet(board->whiteKing, to))
    {
        CLEAR_BIT(board->castlingRights, 3);
        CLEAR_BIT(board->castlingRights, 2);
        // board->whiteCanCastleQueenside = false;
    }
    else if (bitboardIsBitSet(board->blackKing, to))
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
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);

    // Save undo info before making changes
    UndoInfo undoInfo = saveUndoInfo(board, NO_PIECE);
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
    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);

    // Determine the piece type being moved
    PieceType movingPiece = NO_PIECE;
    movingPiece = getPieceAtSquare(board, from);

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
    Square to = TO_SQ(move);
    MoveFlag flag = MOVE_FLAG(move);

    // Save undo info before making changes
    UndoInfo undoInfo = saveUndoInfo(board, NO_PIECE);
    undoInfo.previousZobristKey = board->zobristKey;

    // Remove old EP square from hash
    zobristToggleEnPassant(&board->zobristKey, board, board->epSquare);

    // Save old castling rights
    uint8_t oldCastlingRights = board->castlingRights;

    if (flag == KINGSIDE_CASTLE)
    {
        if (board->sideToMove == WHITE)
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
    }
    else // QUEENSIDE_CASTLE
    {
        if (board->sideToMove == WHITE)
        {
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
        else
        {
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

void unmakeCastlingMove(CBoard *board, MoveFlag flag)
{
    // Unmake castling
    if (flag == KINGSIDE_CASTLE)
    {
        if (board->sideToMove == WHITE)
        {
            // Move king back from g1 to e1 and rook from f1 to h1
            movePieceOnBoard(board, G1, E1, WHITE);
            movePieceOnBoard(board, F1, H1, WHITE);
            updateOccupanciesForCastling(board, G1, E1, F1, H1, WHITE);
        }
        else
        {
            // Move king back from g8 to e8 and rook from f8 to h8
            movePieceOnBoard(board, G8, E8, BLACK);
            movePieceOnBoard(board, F8, H8, BLACK);
            updateOccupanciesForCastling(board, G8, E8, F8, H8, BLACK);
        }
    }
    else // QUEENSIDE_CASTLE
    {
        if (board->sideToMove == WHITE)
        {
            // Move king back from c1 to e1 and rook from d1 to a1
            movePieceOnBoard(board, C1, E1, WHITE);
            movePieceOnBoard(board, D1, A1, WHITE);
            updateOccupanciesForCastling(board, C1, E1, D1, A1, WHITE);
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
        unmakeCastlingMove(board, flag);
    }
    else if (flag >= KNIGHT_PROMO_QUIET && flag <= QUEEN_PROMO_CAPTURE)
    {
        // Unmake promotion
        PieceType promotionPiece = getPromotionPieceType(move);
        bool isPromotionCapture = (flag >= KNIGHT_PROMO_CAPTURE && flag <= QUEEN_PROMO_CAPTURE);
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
    else if (flag == EP_CAPTURE)
    {
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
        if (flag == CAPTURE && undoInfo.capturedPiece != NO_PIECE)
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
