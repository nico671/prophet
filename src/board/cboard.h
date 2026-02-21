
#ifndef CBOARD_H
#define CBOARD_H

#include <stdint.h>

#include "core/bitboard.h"
#include "core/chess_types.h"

/**
 * CBoard struct represents the state of a chess board using bitboards for piece placements and additional fields for game state information such as side to move, castling rights, en passant square, halfmove clock, fullmove number, and a Zobrist hash key for efficient position hashing.
 * FIELD DESCRIPTIONS:
 * - whitePawns, whiteKnights, whiteBishops, whiteRooks, whiteQueens, whiteKing: Bitboards representing the positions of each type of white piece.
 * - blackPawns, blackKnights, blackBishops, blackRooks, blackQueens, blackKing: Bitboards representing the positions of each type of black piece.
 * - whitePieces: Bitboard representing the positions of all white pieces (derived from individual piece bitboards).
 * - blackPieces: Bitboard representing the positions of all black pieces (derived from individual piece bitboards).
 * - allPieces: Bitboard representing the positions of all pieces on the board (derived from whitePieces and blackPieces).
 * - sideToMove: Indicates which player's turn it is (WHITE or BLACK).
 * - castlingRights: A bitfield representing the castling rights for both sides (bits 0-3 correspond to black queenside, black kingside, white queenside, white kingside).
 * - epSquare: The square index (0-63) of the en passant target square if available, or NO_SQUARE if no en passant capture is possible.
 * - halfmoveClock: A counter for the fifty-move rule, incremented after each move and reset to zero after a pawn move or capture.
 * - fullmoveNumber: A counter for the number of full moves in the game, starting at 1 and incremented after Black's move.
 */
typedef struct CBoard
{
    // White Piece bitboards
    Bitboard whitePawns;
    Bitboard whiteKnights;
    Bitboard whiteBishops;
    Bitboard whiteRooks;
    Bitboard whiteQueens;
    Bitboard whiteKing;

    // Black Piece bitboards
    Bitboard blackPawns;
    Bitboard blackKnights;
    Bitboard blackBishops;
    Bitboard blackRooks;
    Bitboard blackQueens;
    Bitboard blackKing;

    // Occupancy bitboards
    Bitboard whitePieces; // all white pieces
    Bitboard blackPieces; // all black pieces
    Bitboard allPieces;   // all pieces

    // Game state info
    Color sideToMove;        // 0 for White, 1 for Black
    uint8_t castlingRights;  // bit meanings: 0 = blackqueenside, 1 = blackkingside, 2 = whitequeenside, 3 = whitekingside
    uint8_t epSquare;        // square index (0-63) or 64 if no en passant available
    uint16_t halfmoveClock;  // for fifty-move rule
    uint16_t fullmoveNumber; // starts at 1, incremented after Black's move

    // zobrist key
    uint64_t zobristKey;
} CBoard;

/**
 * Prints a human-readable representation of a chess board to standard output, displaying the piece layout from rank 8 to rank 1 using standard chess notation characters (uppercase for white, lowercase for black, '.' for empty squares), followed by game state information including side to move, en passant square, halfmove clock, fullmove number, castling rights, and the Zobrist hash key.
 *
 * Returns early with an error message if the provided CBoard pointer is NULL.
 */
void printBoard(CBoard *board);

// Parses a FEN string and initializes a CBoard struct with the corresponding piece placements, game state information (side to move, castling rights, en passant square, halfmove clock, fullmove number), and computes the Zobrist hash key for the resulting board state. The function handles standard FEN formatting and returns the initialized CBoard struct.
CBoard fenToCBoard(const char *fenString);

// Converts a CBoard struct back into a FEN string representation, including piece placements, side to move, castling rights, en passant square, halfmove clock, fullmove number, and returns the resulting FEN string. The caller is responsible for freeing the returned string after use.
char *CBoardToFen(CBoard *board);

#endif // CBOARD_H