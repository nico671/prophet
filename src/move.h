#include "bitboard.h"

// move structure definition
#ifndef MOVE_H
#define MOVE_H

typedef enum
{

    Capture,
    EnPassantCapture,
    KnightPromotion,
    BishopPromotion,
    RookPromotion,
    QueenPromotion,
    KnightPromotionCapture,
    BishopPromotionCapture,
    RookPromotionCapture,
    QueenPromotionCapture,
    Quiet,
    CastleKing,
    CastleQueen,
    Null,
} MoveType;

typedef struct
{
    Square fromSquare; // 0-63
    Square toSquare;   // 0-63
    MoveType type;
} Move;
// Move list structure
typedef struct
{
    Move moves[256]; // Maximum possible moves in a position
    int count;
} MoveList;
#endif // MOVE_H