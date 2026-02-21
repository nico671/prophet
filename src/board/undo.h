#ifndef PROPHET_UNDO_H
#define PROPHET_UNDO_H

#include <stdint.h>

#include "core/chess_types.h"

/**
 * UndoInfo struct is used to store the necessary information to undo a move on the chess board. It captures the piece that was captured (if any), the previous en passant square, the previous halfmove clock for the fifty-move rule, the previous castling rights, and the previous Zobrist hash key.
 * FIELD DESCRIPTIONS:
 * - capturedPiece: The type of piece that was captured by the move being undone (NO_PIECE if no piece was captured).
 * - previousEpSquare: The en passant target square before the move was made (or NO_SQUARE if no en passant capture was possible).
 * - previousHalfmoveClock: The value of the halfmove clock before the move was made, used for tracking the fifty-move rule.
 * - previousCastlingRights: A bitfield representing the castling rights before the move was made (bits 0-3 correspond to black queenside, black kingside, white queenside, white kingside).
 * - previousZobristKey: The Zobrist hash key of the board position before the move was made, allowing for efficient position hashing when undoing moves.
 */
typedef struct UndoInfo
{
    PieceType capturedPiece;        // What was captured (NO_PIECE if none)
    uint8_t previousEpSquare;       // Previous en passant square (or NO_SQUARE)
    uint16_t previousHalfmoveClock; // Previous 50-move counter
    uint8_t previousCastlingRights; // 0..15 bitfield
    uint64_t previousZobristKey;
} UndoInfo;

#endif // PROPHET_UNDO_H
