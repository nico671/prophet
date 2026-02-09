
#ifndef SLIDING_ATTACKS_H
#define SLIDING_ATTACKS_H
#include "core/bitboard.h"
// Large lookup tables live in the .c file (single definition), only declared here.
extern const Bitboard rook_occupancy_maps[64];
extern const Bitboard bishop_occupancy_maps[64];
extern const Bitboard RMagic[64];
extern const Bitboard BMagic[64];
extern const int RBits[64];
extern const int BBits[64];

// Attack tables are materialized once in `initSlidingAttacks()`.
extern Bitboard rook_attacks[64][4096];
extern Bitboard bishop_attacks[64][512];

// Function declarations
void initSlidingAttacks(void);

// Transform occupancy to index using magic.
// Kept in the header so the attack getters can be fully inlined.
static inline int transform(Bitboard occupancy, Bitboard magic, int bits)
{
	return (int)((occupancy * magic) >> (64 - bits));
}

// Fast lookup helpers (inlined for speed in move generation).
static inline Bitboard getRookAttacks(int square, Bitboard occupancy)
{
	occupancy &= rook_occupancy_maps[square];
	int index = transform(occupancy, RMagic[square], RBits[square]);
	return rook_attacks[square][index];
}

static inline Bitboard getBishopAttacks(int square, Bitboard occupancy)
{
	occupancy &= bishop_occupancy_maps[square];
	int index = transform(occupancy, BMagic[square], BBits[square]);
	return bishop_attacks[square][index];
}

static inline Bitboard getQueenAttacks(int square, Bitboard occupancy)
{
	return getRookAttacks(square, occupancy) | getBishopAttacks(square, occupancy);
}

Bitboard generateBishopAttacks(int square, Bitboard blockers); // For testing
Bitboard generateRookAttacks(int square, Bitboard blockers);   // For testing

#endif