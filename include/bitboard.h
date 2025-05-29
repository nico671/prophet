#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>
typedef uint64_t Bitboard;

// map (rank 0-7, file 0-7) → bit index 0-63
static inline int square_index(int rank, int file)
{
    return rank * 8 + file;
}

// build a bitboard with exactly that square set
static inline Bitboard BB(int rank, int file)
{
    return (Bitboard)1 << square_index(rank, file);
}

// Test whether bitboard b has bit sq set (0 or 1)
static inline int bb_test(Bitboard b, int sq)
{
    return (b >> sq) & 1;
}

// Set bit sq in *b
static inline void bb_set(Bitboard *b, int sq)
{
    *b |= (Bitboard)1 << sq;
}

// Clear bit sq in *b
static inline void bb_clear(Bitboard *b, int sq)
{
    *b &= ~((Bitboard)1 << sq);
}

// Pop & return index of the least‐significant 1-bit
static inline int bb_pop_lsb(Bitboard *b)
{
    int idx = __builtin_ctzll(*b);
    *b &= *b - 1;
    return idx;
}

#endif // BITBOARD_H

extern const Bitboard RANK_MASK[8];
extern const Bitboard FILE_MASK[8];