#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <stdint.h>
#include "utils/prng.h"
#include "core/bitboard.h"
#include "core/chess_types.h"
#include "board/cboard.h"

// 0-5: White Pawn, Knight, Bishop, Rook, Queen, King
// 6-11: Black Pawn, Knight, Bishop, Rook, Queen, King
extern uint64_t piece_keys[12][64];
extern uint64_t side_key;
extern uint64_t castle_keys[16];
extern uint64_t en_passant_keys[8];

// Initialize zobrist keys
void initZobristKeys();

// Compute zobrist key from scratch
void computeZobristKey(CBoard *board);

// Helper functions for incremental zobrist updates
static inline int getPieceIndex(PieceType piece, Color color)
{
    return piece + (color == BLACK ? 6 : 0);
}

/**
 * Toggles (XOR) a Zobrist hash key for a specific piece of a given type, color, and square, using precomputed random keys from the piece_keys table.
 *
 * It includes defensive bounds checks to ensure the square is valid (less than 64) and the computed piece index is within the valid range (less than 12) before modifying the key.
 */
static inline void zobristTogglePiece(uint64_t *key, PieceType piece, Color color, Square square)
{
    // Defensive guard: NO_SQUARE should never be toggled.
    if ((unsigned)square < 64U)
    {
        int idx = getPieceIndex(piece, color);
        if ((unsigned)idx < 12U)
            *key ^= piece_keys[idx][square];
    }
}

/**
 * Toggles the side-to-move component of a Zobrist hash key by XORing it with the precomputed side_key value.
 * Used to incrementally update the position's hash key when switching between white's and black's turn.
 */
static inline void zobristToggleSide(uint64_t *key)
{
    *key ^= side_key;
}

/**
 * Updates a Zobrist hash key to reflect a change in castling rights by XOR-ing out the old castling rights and XOR-ing in the new castling rights, using precomputed random keys indexed by the lower 4 bits of each rights value.
 */
static inline void zobristToggleCastling(uint64_t *key, uint8_t oldRights, uint8_t newRights)
{
    *key ^= castle_keys[oldRights & 0x0F];
    *key ^= castle_keys[newRights & 0x0F];
}

// Toggle en-passant hash only when the ep square is capturable by side-to-move.
// This keeps position hashing aligned with repetition semantics.
void zobristToggleEnPassant(uint64_t *key, const CBoard *board, Square epSquare);

#endif // ZOBRIST_H