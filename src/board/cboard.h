
#ifndef CBOARD_H
#define CBOARD_H

#include <stdint.h>

#include "core/bitboard.h"
#include "core/chess_types.h"

/**
 * @struct CBoard
 * @brief Complete chess position state used by movegen/search/eval.
 *
 * Stores per-piece bitboards, cached occupancies, side-to-move and rule metadata,
 * plus an incremental Zobrist hash.
 *
 * Field groups:
 *
 * - White piece bitboards: whitePawns, whiteKnights, whiteBishops, whiteRooks, whiteQueens, whiteKing
 *
 * - Black piece bitboards: blackPawns, blackKnights, blackBishops, blackRooks, blackQueens, blackKing
 *
 * - Occupancies: whitePieces, blackPieces, allPieces
 *
 * - Square piece mapping: pieceAtSquare[64] for O(1) piece type retrieval at any square without bitboard checks
 *
 * - State metadata: sideToMove, castlingRights, epSquare, halfmoveClock, fullmoveNumber
 *
 * - Hash: zobristKey
 */
typedef struct CBoard {
    // --- Piece Placements ---
    Bitboard whitePawns; /**< Bitboard: White pawns */
    Bitboard whiteKnights; /**< Bitboard: White knights */
    Bitboard whiteBishops; /**< Bitboard: White bishops */
    Bitboard whiteRooks; /**< Bitboard: White rooks */
    Bitboard whiteQueens; /**< Bitboard: White queens */
    Bitboard whiteKing; /**< Bitboard: White king */

    Bitboard blackPawns; /**< Bitboard: Black pawns */
    Bitboard blackKnights; /**< Bitboard: Black knights */
    Bitboard blackBishops; /**< Bitboard: Black bishops */
    Bitboard blackRooks; /**< Bitboard: Black rooks */
    Bitboard blackQueens; /**< Bitboard: Black queens */
    Bitboard blackKing; /**< Bitboard: Black king */

    // --- Occupancy Cache ---
    Bitboard whitePieces; /**< Combined bitboard of all white pieces */
    Bitboard blackPieces; /**< Combined bitboard of all black pieces */
    Bitboard allPieces; /**< Combined bitboard of every piece on board */

    PieceType pieceAtSquare[64]; /**< Array mapping each square index to the piece type occupying it, or NONE if empty */

    // --- Game State Metadata ---
    Color sideToMove; /** 0 for White, 1 for Black */

    /**
     * @brief Castling rights bitfield.
     * Bits: 0:BQ, 1:BK, 2:WQ, 3:WK
     */
    uint8_t castlingRights;

    uint8_t epSquare; /**< Index (0-63) or 64 if none available */
    uint16_t halfmoveClock; /**< Counter for the fifty-move rule */
    uint16_t fullmoveNumber; /**< Incremented after every Black move */

    // --- Optimization ---
    uint64_t zobristKey; /**< Unique hash key for the current position */
} CBoard;

/**
 * @brief Prints a human-readable representation of a chess board to standard output, displaying the piece layout from rank 8 to rank 1 using standard chess notation characters (uppercase for white, lowercase for black, '.' for empty squares), followed by game state information including side to move, en passant square, halfmove clock, fullmove number, castling rights, and the Zobrist hash key.
 *
 * Returns early with an error message if the provided CBoard pointer is NULL.
 *
 * @param board Pointer to the CBoard struct to print.
 */
void printBoard(CBoard* board);

/**
 * @brief Parses a FEN string and initializes a CBoard struct with the corresponding piece placements, game state information (side to move, castling rights, en passant square, halfmove clock, fullmove number), and computes the Zobrist hash key for the resulting board state.
 * The function validates the FEN string format and returns false if the string is invalid (e.g., incorrect piece placement, missing sections, invalid characters). On success, it populates the provided CBoard struct with the parsed information and returns true.
 *
 * @param fenString A null-terminated string in Forsyth-Edwards Notation representing a chess position, including piece placements, side to move, castling rights, en passant square, halfmove clock, and fullmove number.
 * @param board Pointer to a CBoard struct that will be initialized based on the provided FEN string. The function will populate all fields of the CBoard struct according to the information encoded in the FEN string.
 * @return bool True if the FEN string was successfully parsed and the CBoard struct was initialized, false otherwise.
 */
bool fenToCBoard(const char* fenString, CBoard* board);

/**
 * @brief Converts a CBoard struct back into a FEN string representation, including piece placements, side to move, castling rights, en passant square, halfmove clock, and fullmove number. The returned string is dynamically allocated and should be freed by the caller.
 *
 * @param board Pointer to the CBoard struct to convert to FEN.
 * @return char* Dynamically allocated null-terminated string in FEN format representing the given CBoard position. Caller is responsible for freeing this memory.
 */
char* CBoardToFen(CBoard* board);

/**
 * @brief Returns the piece type occupying the given square on the board, or NO_PIECE if the square is empty. The function uses the pieceAtSquare array in the CBoard struct, which is indexed by the square (0-63 corresponding to A1-H8) and contains the piece type for each square. This allows for O(1) retrieval of the piece type at any given square without needing to check individual bitboards.
 *
 * @param board Pointer to the CBoard struct representing the current board state.
 * @param square The square for which to retrieve the piece type.
 * @return PieceType The piece type occupying the given square, or NO_PIECE if the square is empty.
 */
PieceType getPieceAtSquare(const CBoard* board, Square square);

/**
 * @brief Returns a pointer to the bitboard representing the specified piece for the given color.
 *
 * @param board Pointer to the CBoard struct.
 * @param color The color of the pieces to retrieve.
 * @param piece The type of piece to retrieve.
 * @return Bitboard* Pointer to the bitboard representing the specified piece for the given color, or NULL if not found.
 */
Bitboard* pieceBitboard(CBoard* board, Color color, PieceType piece);

/**
 * @brief Adds a piece to the board at the specified square.
 *
 * @param board Pointer to the CBoard struct.
 * @param square The square where the piece will be added.
 * @param color The color of the piece to add.
 * @param piece The type of piece to add.
 */
void addPieceToBoard(CBoard* board, Square square, Color color, PieceType piece);

/**
 * @brief Removes a piece from the board at the specified square.
 *
 * @param board Pointer to the CBoard struct.
 * @param square The square from which the piece will be removed.
 * @param color The color of the piece to remove.
 * @param piece The type of piece to remove.
 */
void removePieceFromBoard(CBoard* board, Square square, Color color, PieceType piece);

/**
 * @brief Moves a piece from one square to another on the board.
 *
 * @param board Pointer to the CBoard struct.
 * @param from The square from which the piece will be moved.
 * @param to The square to which the piece will be moved.
 * @param side The color of the piece being moved.

*/
void movePieceOnBoard(CBoard* board, Square from, Square to, Color side);

/**
 * @brief Removes a captured piece from the board and returns its type.
 *
 * @param board Pointer to the CBoard struct.
 * @param square The square from which the piece will be removed.
 * @param capturingColor The color of the piece that captured the target piece.
 * @return PieceType The type of the captured piece, or NO_PIECE if no piece was captured.

 */
PieceType removeCapturedPiece(CBoard* board, Square square, Color capturingColor);

/**
 * @brief Updates the occupancy bitboards when moving a piece from one square to another.
 *
 * @param board Pointer to the CBoard struct.
 * @param from The square from which the piece will be moved.
 * @param to The square to which the piece will be moved.
 * @param color The color of the piece being moved.
 */
void updateOccupanciesForMove(CBoard* board, Square from, Square to, Color color);

/**
 * @brief Updates the occupancy bitboards when capturing a piece.
 *
 * @param board Pointer to the CBoard struct.
 * @param square The square from which the piece will be captured.
 * @param capturedColor The color of the piece being captured.
 */
void updateOccupanciesForCapture(CBoard* board, Square square, Color capturedColor);

/**
 * @brief Updates the occupancy bitboards when promoting a pawn. Same as a regular move - color occupancy changes from 'from' to 'to'
 *
 * @param board Pointer to the CBoard struct.
 * @param from The square from which the pawn will be moved.
 * @param to The square to which the pawn will be moved.
 * @param color The color of the piece being moved.
 */
void updateOccupanciesForPromotion(CBoard* board, Square from, Square to, Color color);

/**
 * @brief Updates the occupancy bitboards when castling.
 *
 * @param board Pointer to the CBoard struct.
 * @param kingFrom The square from which the king will be moved.
 * @param kingTo The square to which the king will be moved.
 * @param rookFrom The square from which the rook will be moved.
 * @param rookTo The square to which the rook will be moved.
 * @param color The color of the pieces being moved.
 */
void updateOccupanciesForCastling(CBoard* board, Square kingFrom, Square kingTo, Square rookFrom, Square rookTo, Color color);

/**
 * @brief Updates the castling rights when a rook or king moves.
 *
 * @param board Pointer to the CBoard struct.
 * @param from The square from which the piece will be moved.
 * @param to The square to which the piece will be moved.
 */
void updateCastlingRights(CBoard* board, Square from, Square to);
#endif // CBOARD_H
