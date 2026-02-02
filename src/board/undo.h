#ifndef PROPHET_UNDO_H
#define PROPHET_UNDO_H

#include <stdint.h>

#include "core/chess_types.h"

// Snapshot of state needed to unmake a move.
// Stored/returned by value (small POD), so keep it lightweight.
typedef struct UndoInfo
{
    PieceType capturedPiece;        // What was captured (NO_PIECE if none)
    uint8_t previousEpSquare;       // Previous en passant square (or NO_SQUARE)
    uint16_t previousHalfmoveClock; // Previous 50-move counter
    uint8_t previousCastlingRights; // 0..15 bitfield
    uint64_t previousZobristKey;
} UndoInfo;

#endif // PROPHET_UNDO_H
