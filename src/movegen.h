#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"
#include "cboard.h"

// Future Move representation plan:
// Bits 0-5:   FROM square (6 bits = 0-63)
// Bits 6-11:  TO square (6 bits = 0-63)
// Bits 12-15: FLAGS (4 bits = 16 possible types)
// Flag bits 12-15:

// 0000 (0)  = Quiet move
// 0001 (1)  = Double pawn push
// 0010 (2)  = Kingside castle
// 0011 (3)  = Queenside castle
// 0100 (4)  = Capture
// 0101 (5)  = EP capture

// 1000 (8)  = Knight promotion (quiet)
// 1001 (9)  = Bishop promotion (quiet)
// 1010 (10) = Rook promotion (quiet)
// 1011 (11) = Queen promotion (quiet)

// 1100 (12) = Knight promotion capture
// 1101 (13) = Bishop promotion capture
// 1110 (14) = Rook promotion capture
// 1111 (15) = Queen promotion capture
// typedef uint16_t Move;

// move flag macros
// #define IS_CAPTURE(m) (((m) >> 12) & 0x4)   // bit 2 set
// #define IS_PROMOTION(m) (((m) >> 12) & 0x8) // bit 3 set
// #define IS_CASTLE(m) ((((m) >> 12) & 0xE) == 0x2)
// #define IS_EP(m) (((m) >> 12) == 5)

// move flag
typedef enum
{
    QUIET = 0,
    DOUBLE_PAWN_PUSH = 1,
    KINGSIDE_CASTLE = 2,
    QUEENSIDE_CASTLE = 3,
    CAPTURE = 4,
    EP_CAPTURE = 5,
    KNIGHT_PROMO_QUIET = 8,
    BISHOP_PROMO_QUIET = 9,
    ROOK_PROMO_QUIET = 10,
    QUEEN_PROMO_QUIET = 11,
    KNIGHT_PROMO_CAPTURE = 12,
    BISHOP_PROMO_CAPTURE = 13,
    ROOK_PROMO_CAPTURE = 14,
    QUEEN_PROMO_CAPTURE = 15,
} MoveFlag;

typedef struct
{
    Square from;
    Square to;
    MoveFlag flag;
} Move;

// Move list structure
typedef struct
{
    Move moves[256]; // Maximum possible moves in a position
    int count;
} MoveList;

// // Move construction and extraction
// #define MOVE(from, to, flags) ((Move)((from) | ((to) << 6) | ((flags) << 12)))
// #define FROM_SQ(move) ((move) & 0x3F)
// #define TO_SQ(move) (((move) >> 6) & 0x3F)
// #define MOVE_FLAGS(move) ((move) >> 12)
// #define IS_CAPTURE(move) (MOVE_FLAGS(move) & CAPTURE)
// #define IS_PROMOTION(move) (MOVE_FLAGS(move) & PROMOTION)

// // Move generation functions
// void generateAllMoves(const CBoard *board, MoveList *moveList);
// void generateCaptures(const CBoard *board, MoveList *moveList);
// int isSquareAttacked(const CBoard *board, int square, Color attackingColor);
// int isInCheck(const CBoard *board, Color color);

// // Helper functions for move generation
// void generatePawnMoves(const CBoard *board, MoveList *moveList);
// void generateKnightMoves(const CBoard *board, MoveList *moveList);
// void generateBishopMoves(const CBoard *board, MoveList *moveList);
// void generateRookMoves(const CBoard *board, MoveList *moveList);
// void generateQueenMoves(const CBoard *board, MoveList *moveList);
// void generateKingMoves(const CBoard *board, MoveList *moveList);
// void generateCastlingMoves(const CBoard *board, MoveList *moveList);

// // Move validation
// int isMoveLegal(const CBoard *board, Move move);

// // Utility functions
// void printMove(Move move);
// void printMoveList(const MoveList *moveList);

#endif // MOVEGEN_H
