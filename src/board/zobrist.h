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

/**
 * @brief Initializes the Zobrist hashing keys used for board position hashing, including keys for each piece on each square, side to move, castling rights (16 combinations), and en passant files, using a deterministic pseudo-random number generator with a fixed seed to ensure reproducibility.
 * @note The function is guarded to run only once via a zobristKeysInitialized flag.
 *
 */
void initZobristKeys();

/**
 * @brief Computes the Zobrist hash key for a given board position.
 *
 * @param board The board position for which to compute the Zobrist key.
 */
void computeZobristKey(CBoard *board);

/**
 * @brief Determines whether the en passant square should be included in the Zobrist hash key based on whether it is a valid en passant target square that can be captured by the opponent's pawn on the next move.
 * This function checks if the epSquare is within bounds, and if there is an opponent pawn in position to capture en passant.
 *
 * @param piece The piece type (PAWN, KNIGHT, etc.)
 * @param color The color of the piece (WHITE or BLACK)
 * @return int
 */
static inline int getPieceIndex(PieceType piece, Color color)
{
    return piece + (color == BLACK ? 6 : 0);
}

/**
 * @brief Toggles (XOR) a Zobrist hash key for a specific piece of a given type, color, and square, using precomputed random keys from the piece_keys table.
 *
 * @param key The Zobrist hash key to be toggled.
 * @param piece The type of the piece (PAWN, KNIGHT, etc.).
 * @param color The color of the piece (WHITE or BLACK).
 * @param square The square on which the piece is located.
 * @note This function includes a defensive guard to prevent toggling for NO_SQUARE and ensures that the piece index is within bounds.
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
 * @brief Toggles the side-to-move component of a Zobrist hash key by XORing it with the precomputed side_key value.
 * Used to incrementally update the position's hash key when switching between white's and black's turn.
 *
 * @param key The Zobrist hash key to be toggled.
 */
static inline void zobristToggleSide(uint64_t *key)
{
    *key ^= side_key;
}

/**
 * @brief Toggles the castling rights component of a Zobrist hash key by XORing it with precomputed random keys indexed by the lower 4 bits of each rights value.
 *
 * @param key The Zobrist hash key to be toggled.
 * @param oldRights The old castling rights value.
 * @param newRights The new castling rights value.
 */
static inline void zobristToggleCastling(uint64_t *key, uint8_t oldRights, uint8_t newRights)
{
    *key ^= castle_keys[oldRights & 0x0F];
    *key ^= castle_keys[newRights & 0x0F];
}

/**
 * @brief Toggles the en passant component of a Zobrist hash key by XORing it with a precomputed random key indexed by the en passant file.
 * This function is only called when the en passant square is capturable by the side to move, ensuring that position hashing remains consistent with repetition semantics.
 *
 * @param key The Zobrist hash key to be toggled.
 * @param board The board position for which to compute the Zobrist key.
 * @param epSquare The en passant square to be toggled in the hash key.
 */
void zobristToggleEnPassant(uint64_t *key, const CBoard *board, Square epSquare);

#endif // ZOBRIST_H