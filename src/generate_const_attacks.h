#include "bitboard.h"
#include <stdio.h>

extern const Bitboard knight_attacks[64];
void initKnightAttacks();
Bitboard getKnightAttacks(int square);

extern const Bitboard king_attacks[64];
void initKingAttacks();
Bitboard getKingAttacks(int square);

// extern const Bitboard white_pawn_attacks[64];

// void initWhitePawnAttacks();
extern const Bitboard white_pawn_attacks[64];
Bitboard getWhitePawnAttacks(int square);
Bitboard getBlackPawnAttacks(int square);
extern const Bitboard black_pawn_attacks[64];