
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
 * @param fenString A null-terminated string in Forsyth-Edwards Notation representing a chess position, including piece placements, side to move, castling rights, en passant square, halfmove clock, and fullmove number.
 * @return CBoard Initialized CBoard struct representing the position described by the FEN string.
 */
CBoard fenToCBoard(const char *fenString);

/**
 * @brief Converts a CBoard struct back into a FEN string representation, including piece placements, side to move, castling rights, en passant square, halfmove clock, and fullmove number. The returned string is dynamically allocated and should be freed by the caller.
 *
 * @param board Pointer to the CBoard struct to convert to FEN.
 * @return char* Dynamically allocated null-terminated string in FEN format representing the given CBoard position. Caller is responsible for freeing this memory.
 */
char *CBoardToFen(CBoard *board);

#endif // CBOARD_H