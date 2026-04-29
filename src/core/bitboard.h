#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "chess_types.h"

// A1 = LSB, H8 = MSB, little-endian rank-file mapping
typedef uint64_t Bitboard;

/**
 * @brief Checks if a bitboard is empty (i.e., all bits are zero).
 *
 * @param bb The bitboard to check.
 * @return true if the bitboard is empty, false otherwise.
 */
static inline bool bitboard_is_empty(Bitboard bb)
{
    return bb == 0ULL;
}

static inline int bitboard_popcount(Bitboard bb)
{
    return __builtin_popcountll(bb);
}

/**
 * @brief Returns the index (0-63) of the least significant 1 bit in a bitboard.
 *
 * @param bb The bitboard to analyze.
 * @return int The index of the least significant 1 bit.
 */
static inline int bitboard_lsb_index(Bitboard bb)
{
    return __builtin_ctzll(bb);
}

/**
 * @brief Pops and returns the index (0-63) of the least significant 1 bit in a bitboard.
 *        The bitboard is modified in place by clearing the least significant 1 bit.
 *
 * @param bb The bitboard to pop from.
 * @return int The index of the least significant 1 bit, or NO_SQUARE if the bitboard is empty.
 */
static inline int bitboard_pop_lsb(Bitboard* bb)
{
    if (bitboard_is_empty(*bb))
        return NO_SQUARE;
    int idx = __builtin_ctzll(*bb); // count trailing zeros, built-in function
    *bb &= *bb - 1;
    return idx;
}

/**
 * @brief Returns a bitboard with a single 1-bit set at the given square index.
 *
 * @param sq The square index (0-63).
 * @return Bitboard The resulting bitboard.
 */
static inline Bitboard bitboard_square_mask(Square sq)
{
    return (Bitboard)1ULL << sq;
}

/**
 * @brief Sets the bit corresponding to a square in a bitboard.
 *
 * @param bb The bitboard to modify.
 * @param sq The square index (0-63) to set.
 */
static inline void bitboard_set_square_bit(Bitboard* bb, Square sq)
{
    *bb |= bitboard_square_mask(sq);
}

/**
 * @brief Clears the bit corresponding to a square in a bitboard.
 *
 * @param bb The bitboard to modify.
 * @param sq The square index (0-63) to clear.
 */
static inline void bitboard_clear_square_bit(Bitboard* bb, int sq)
{
    *bb &= ~bitboard_square_mask(sq);
}

/* rank masks (1..8) */
static const Bitboard RANK_1 = 0x00000000000000FFULL;
static const Bitboard RANK_2 = RANK_1 << 8;
static const Bitboard RANK_7 = RANK_1 << 48;
static const Bitboard RANK_8 = RANK_1 << 56;

/**
 * @brief Tests whether a bitboard has a specific bit set.
 *
 * @param bb The bitboard to test.
 * @param sq The square index (0-63) to test.
 * @return int 1 if the bit is set, 0 otherwise.
 */
static inline int bitboard_is_bit_set(Bitboard bb, int sq)
{
    return (int)((bb >> sq) & 1ULL);
}

/**
 * @brief Shifts a bitboard one rank north (toward rank 8).
 *
 * @param bb The bitboard to shift.
 * @return Bitboard The shifted bitboard.
 */
static inline Bitboard bitboard_shift_north(Bitboard bb)
{
    return bb << 8;
}

/**
 * @brief Shifts a bitboard one rank south (toward rank 1).
 *
 * @param bb The bitboard to shift.
 * @return Bitboard The shifted bitboard.
 */
static inline Bitboard bitboard_shift_south(Bitboard bb) { return bb >> 8; }

#endif // BITBOARD_H