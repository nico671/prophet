#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>
#include <stdbool.h>

#include "chess_types.h"

// A1 = LSB, H8 = MSB, little-endian rank-file mapping
typedef unsigned long long Bitboard;

/**
 * @brief Checks if a bitboard is empty (i.e., all bits are zero).
 *
 * @param bb The bitboard to check.
 * @return true if the bitboard is empty, false otherwise.
 */
static inline bool bitboardIsEmpty(Bitboard bb)
{
    return bb == 0ULL;
}

// static inline int bitBoardPopcount(Bitboard bb)
// {
//     return __builtin_popcountll(bb);
// }

/**
 * @brief Returns the index (0-63) of the least significant 1 bit in a bitboard.
 *
 * @param bb The bitboard to analyze.
 * @return int The index of the least significant 1 bit.
 */
static inline int bitboardLSBIndex(Bitboard bb)
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
static inline int bitboardPopLSB(Bitboard *bb)
{
    if (bitboardIsEmpty(*bb))
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
static inline Bitboard bitboardSquareMask(Square sq)
{
    return (Bitboard)1ULL << sq;
}

/**
 * @brief Sets the bit corresponding to a square in a bitboard.
 *
 * @param bb The bitboard to modify.
 * @param sq The square index (0-63) to set.
 */
static inline void bitboardSetSquareBit(Bitboard *bb, Square sq)
{
    *bb |= bitboardSquareMask(sq);
}

/**
 * @brief Clears the bit corresponding to a square in a bitboard.
 *
 * @param bb The bitboard to modify.
 * @param sq The square index (0-63) to clear.
 */
static inline void bitboardClearSquareBit(Bitboard *bb, int sq)
{
    *bb &= ~bitboardSquareMask(sq);
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
static inline int bitboardIsBitSet(Bitboard bb, int sq)
{
    return (int)((bb >> sq) & 1ULL);
}

/**
 * @brief Shifts a bitboard one rank north (toward rank 8).
 *
 * @param bb The bitboard to shift.
 * @return Bitboard The shifted bitboard.
 */
static inline Bitboard bitboardShiftNorth(Bitboard bb)
{
    return bb << 8;
}

/**
 * @brief Shifts a bitboard one rank south (toward rank 1).
 *
 * @param bb The bitboard to shift.
 * @return Bitboard The shifted bitboard.
 */
static inline Bitboard bitboardShiftSouth(Bitboard bb) { return bb >> 8; }

// generic bitfield ops, not for bitboards but useful for other bitfield manipulations
#define BIT_MASK(pos) (1ULL << (pos))
#define SET_BIT(var, pos) ((var) |= BIT_MASK(pos))
#define CLEAR_BIT(var, pos) ((var) &= ~BIT_MASK(pos))
#define CHECK_BIT(var, pos) (!!((var) & BIT_MASK(pos)))

#endif // BITBOARD_H