#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "bitboard.h"
#ifndef BOARD_H
#define BOARD_H

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
    Color sideToMove; // true for White, false for Black
    bool whiteCanCastleKingside;
    bool whiteCanCastleQueenside;
    bool blackCanCastleKingside;
    bool blackCanCastleQueenside;
    uint8_t epSquare;        // square index (0-63) or 64 if no en passant available
    uint16_t halfmoveClock;  // for fifty-move rule
    uint16_t fullmoveNumber; // starts at 1, incremented after Black's move

    // king squares
    uint8_t whiteKingSquare;
    uint8_t blackKingSquare;

    // zobrist key
    uint64_t zobristKey;
} CBoard;

void recomputeOccupancies(CBoard *board);

// bool validateBoard(CBoard *board);

void printBoard(CBoard *board);
typedef struct UndoInfo
{
    PieceType capturedPiece;        // What was captured (NO_PIECE if none)
    uint8_t previousEpSquare;       // Previous en passant square
    uint16_t previousHalfmoveClock; // Previous 50-move counter
    bool prevWhiteCastleKingside;   // Previous castling rights
    bool prevWhiteCastleQueenside;
    bool prevBlackCastleKingside;
    bool prevBlackCastleQueenside;
    // uint64_t previousZobristKey;  // TODO: Add when implementing Zobrist
} UndoInfo;
#endif // BOARD_H