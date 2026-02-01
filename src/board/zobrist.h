#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <stdint.h>
#include "utils/prng.h"
#include "core/bitboard.h"
#include "core/chess_types.h"
#include "board/cboard.h"

// Forward declaration
typedef struct CBoard CBoard;

// 0-5: White Pawn, Knight, Bishop, Rook, Queen, King
// 6-11: Black Pawn, Knight, Bishop, Rook, Queen, King
uint64_t piece_keys[12][64];

uint64_t side_key;

//
uint64_t castle_keys[16];

// 8 possible files for En Passant (or 1 for "None")
uint64_t en_passant_keys[8];

// Initialize zobrist keys
void initZobristKeys();

// Compute zobrist key from scratch
void computeZobristKey(CBoard *board);

// Helper functions for incremental zobrist updates
static inline int getPieceIndex(PieceType piece, Color color)
{
    return piece + (color == BLACK ? 6 : 0);
}

static inline void zobristTogglePiece(uint64_t *key, PieceType piece, Color color, Square square)
{
    *key ^= piece_keys[getPieceIndex(piece, color)][square];
}

static inline void zobristToggleSide(uint64_t *key)
{
    *key ^= side_key;
}

static inline void zobristToggleCastling(uint64_t *key, uint8_t oldRights, uint8_t newRights)
{
    *key ^= castle_keys[oldRights];
    *key ^= castle_keys[newRights];
}

static inline void zobristToggleEnPassant(uint64_t *key, Square epSquare)
{
    if (epSquare != NO_SQUARE)
    {
        int file = epSquare % 8;
        *key ^= en_passant_keys[file];
    }
}

#endif // ZOBRIST_H