
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
Bitboard getRookAttacks(int square, Bitboard occupancy);
Bitboard getBishopAttacks(int square, Bitboard occupancy);
Bitboard getQueenAttacks(int square, Bitboard occupancy);
Bitboard generateBishopAttacks(int square, Bitboard blockers); // For testing
Bitboard generateRookAttacks(int square, Bitboard blockers);   // For testing

#endif