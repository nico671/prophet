
#ifndef SLIDING_ATTACKS_H
#define SLIDING_ATTACKS_H
#include "core/bitboard.h"

// Attack tables are materialized once in `initSlidingAttacks()`.
extern Bitboard rook_attacks[64][4096];
extern Bitboard bishop_attacks[64][512];

// Function declarations
void initSlidingAttacks(void);

// Fast lookup helpers.
Bitboard getRookAttacks(int square, Bitboard occupancy);
Bitboard getBishopAttacks(int square, Bitboard occupancy);
Bitboard getQueenAttacks(int square, Bitboard occupancy);

Bitboard generateBishopAttacks(int square, Bitboard blockers); // For testing
Bitboard generateRookAttacks(int square, Bitboard blockers);   // For testing

#endif