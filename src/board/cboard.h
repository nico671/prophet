
#ifndef CBOARD_H
#define CBOARD_H

#include <stdint.h>

#include "core/bitboard.h"
#include "core/chess_types.h"

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
    Color sideToMove;        // true for White, false for Black
    uint8_t castlingRights;  // bit meanings: 0 = blackqueenside, 1 = blackkingside, 2 = whitequeenside, 3 = whitekingside
    uint8_t epSquare;        // square index (0-63) or 64 if no en passant available
    uint16_t halfmoveClock;  // for fifty-move rule
    uint16_t fullmoveNumber; // starts at 1, incremented after Black's move

    // zobrist key
    uint64_t zobristKey;
} CBoard;

void recomputeOccupancies(CBoard *board);

// bool validateBoard(CBoard *board);

void printBoard(CBoard *board);
CBoard fenToCBoard(const char *fenString);
char *CBoardToFen(CBoard *board);

#endif // CBOARD_H