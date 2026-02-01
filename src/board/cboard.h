
#ifndef BOARD_H
#define BOARD_H
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "core/bitboard.h"
#include <stdlib.h>
#include <string.h>
#include "utils/bit_manipulation.h"
#include "zobrist.h"

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
CBoard fenToCBoard(char *fenString);
char *CBoardToFen(CBoard *board);
typedef struct UndoInfo
{
    PieceType capturedPiece;        // What was captured (NO_PIECE if none)
    uint8_t previousEpSquare;       // Previous en passant square
    uint16_t previousHalfmoveClock; // Previous 50-move counter
    uint8_t previousCastlingRights; // bit meanings: 0 = blackqueenside, 1 = blackkingside, 2 = whitequeenside, 3 = whitekingside
    uint64_t previousZobristKey;
} UndoInfo;

#endif // BOARD_H