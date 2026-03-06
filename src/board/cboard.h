
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
 * - State metadata: sideToMove, castlingRights, epSquare, halfmoveClock, fullmoveNumber
 *
 * - Hash: zobristKey
 */
typedef struct CBoard
{
    // --- Piece Placements ---
    Bitboard whitePawns;   /**< Bitboard: White pawns */
    Bitboard whiteKnights; /**< Bitboard: White knights */
    Bitboard whiteBishops; /**< Bitboard: White bishops */
    Bitboard whiteRooks;   /**< Bitboard: White rooks */
    Bitboard whiteQueens;  /**< Bitboard: White queens */
    Bitboard whiteKing;    /**< Bitboard: White king */

    Bitboard blackPawns;   /**< Bitboard: Black pawns */
    Bitboard blackKnights; /**< Bitboard: Black knights */
    Bitboard blackBishops; /**< Bitboard: Black bishops */
    Bitboard blackRooks;   /**< Bitboard: Black rooks */
    Bitboard blackQueens;  /**< Bitboard: Black queens */
    Bitboard blackKing;    /**< Bitboard: Black king */

    // --- Occupancy Cache ---
    Bitboard whitePieces; /**< Combined bitboard of all white pieces */
    Bitboard blackPieces; /**< Combined bitboard of all black pieces */
    Bitboard allPieces;   /**< Combined bitboard of every piece on board */

    PieceType pieceAtSquare[64]; /**< Array mapping each square index to the piece type occupying it, or NONE if empty */

    // --- Game State Metadata ---
    Color sideToMove; /** 0 for White, 1 for Black */

    /**
     * @brief Castling rights bitfield.
     * Bits: 0:BQ, 1:BK, 2:WQ, 3:WK
     */
    uint8_t castlingRights;

    uint8_t epSquare;        /**< Index (0-63) or 64 if none available */
    uint16_t halfmoveClock;  /**< Counter for the fifty-move rule */
    uint16_t fullmoveNumber; /**< Incremented after every Black move */

    // --- Optimization ---
    uint64_t zobristKey; /**< Unique hash key for the current position */
} CBoard;

/**
 * Prints a human-readable representation of a chess board to standard output, displaying the piece layout from rank 8 to rank 1 using standard chess notation characters (uppercase for white, lowercase for black, '.' for empty squares), followed by game state information including side to move, en passant square, halfmove clock, fullmove number, castling rights, and the Zobrist hash key.
 *
 * Returns early with an error message if the provided CBoard pointer is NULL.
 */
void printBoard(CBoard *board);

/**
 * @brief Parses a FEN string and initializes a CBoard struct with the corresponding piece placements, game state information (side to move, castling rights, en passant square, halfmove clock, fullmove number), and computes the Zobrist hash key for the resulting board state.
 *
 * @note Assumes the input FEN string is well-formed and valid. Error handling for invalid FEN strings is a TODO item.
 *
 * @param fenString A null-terminated string in Forsyth-Edwards Notation representing a chess position, including piece placements, side to move, castling rights, en passant square, halfmove clock, and fullmove number.
 * @param board Pointer to a CBoard struct that will be initialized based on the provided FEN string. The function will populate all fields of the CBoard struct according to the information encoded in the FEN string.
 * @return bool True if the FEN string was successfully parsed and the CBoard struct was initialized, false otherwise.
 */
bool fenToCBoard(const char *fenString, CBoard *board);

/**
 * @brief Converts a CBoard struct back into a FEN string representation, including piece placements, side to move, castling rights, en passant square, halfmove clock, and fullmove number. The returned string is dynamically allocated and should be freed by the caller.
 *
 * @param board Pointer to the CBoard struct to convert to FEN.
 * @return char* Dynamically allocated null-terminated string in FEN format representing the given CBoard position. Caller is responsible for freeing this memory.
 */
char *CBoardToFen(CBoard *board);

/**
 * @brief Returns the piece type occupying the given square on the board, or NO_PIECE if the square is empty. The function uses the pieceAtSquare array in the CBoard struct, which is indexed by the square (0-63 corresponding to A1-H8) and contains the piece type for each square. This allows for O(1) retrieval of the piece type at any given square without needing to check individual bitboards.
 *
 * @param board Pointer to the CBoard struct representing the current board state.
 * @param square The square for which to retrieve the piece type.
 * @return PieceType The piece type occupying the given square, or NO_PIECE if the square is empty.
 */
PieceType getPieceAtSquare(const CBoard *board, Square square);
#endif // CBOARD_H