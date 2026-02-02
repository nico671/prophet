#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>
#include <stdbool.h>

#include "chess_types.h"

// A1 = LSB, H8 = MSB, little-endian rank-file mapping
typedef unsigned long long Bitboard;

static inline bool bb_is_empty(Bitboard bb)
{
    return bb == 0ULL;
}

static inline int bb_popcount(Bitboard bb)
{
    return __builtin_popcountll(bb);
}

// returns index (0-63) of least significant 1 bit
// precondition: bb != 0
static inline int bb_lsb_idx(Bitboard bb)
{
    return __builtin_ctzll(bb);
}

// Pop & return index of the least‐significant 1-bit
static inline int bb_pop_lsb(Bitboard *b)
{
    if (bb_is_empty(*b))
        return NO_SQUARE;
    int idx = __builtin_ctzll(*b); // count trailing zeros, built-in function
    *b &= *b - 1;
    return idx;
}

// Build a mask for a single square by index.
static inline Bitboard bb_square(int sq)
{
    return (Bitboard)1ULL << sq;
}

// Set bit sq in *b
static inline void bb_set(Bitboard *b, int sq)
{
    *b |= bb_square(sq);
}

// Clear bit sq in *b
static inline void bb_clear(Bitboard *b, int sq)
{
    *b &= ~bb_square(sq);
}

/* file masks (A, H) and complements */
// static const Bitboard FILE_A = 0x0101010101010101ULL;
// static const Bitboard FILE_H = 0x8080808080808080ULL;
// static const Bitboard NOT_FILE_A = ~FILE_A;
// static const Bitboard NOT_FILE_H = ~FILE_H;

/* rank masks (1..8) */
static const Bitboard RANK_1 = 0x00000000000000FFULL;
static const Bitboard RANK_2 = RANK_1 << 8;
// static const Bitboard RANK_3 = RANK_1 << 16;
// static const Bitboard RANK_4 = RANK_1 << 24;
// static const Bitboard RANK_5 = RANK_1 << 32;
// static const Bitboard RANK_6 = RANK_1 << 40;
static const Bitboard RANK_7 = RANK_1 << 48;
static const Bitboard RANK_8 = RANK_1 << 56;

// Test whether bitboard b has bit sq set (0 or 1)
static inline int is_bit_set(Bitboard b, int sq)
{
    return (int)((b >> sq) & 1ULL);
}

/* Safe directional shifts (no wraps) */
static inline Bitboard north(Bitboard b)
{
    return b << 8;
}
static inline Bitboard south(Bitboard b) { return b >> 8; }
// static inline Bitboard east(Bitboard b) { return (b << 1) & NOT_FILE_A; } // file++ (mask out wraps into file a)
// static inline Bitboard west(Bitboard b) { return (b >> 1) & NOT_FILE_H; } // file-- (mask out wraps into file h)
// static inline Bitboard north_east(Bitboard b) { return (b << 9) & NOT_FILE_A; }
// static inline Bitboard north_west(Bitboard b) { return (b << 7) & NOT_FILE_H; }
// static inline Bitboard south_east(Bitboard b) { return (b >> 7) & NOT_FILE_A; }
// static inline Bitboard south_west(Bitboard b) { return (b >> 9) & NOT_FILE_H; }

// generic bitfield ops
#define BIT_MASK(pos) (1ULL << (pos))

#define SET_BIT(var, pos) ((var) |= BIT_MASK(pos))
#define CLEAR_BIT(var, pos) ((var) &= ~BIT_MASK(pos))
#define CHECK_BIT(var, pos) (!!((var) & BIT_MASK(pos)))
#define TOGGLE_BIT(var, pos) ((var) ^= BIT_MASK(pos))

#endif // BITBOARD_H