#include "bitboard.h"
#include <stdio.h>

extern const Bitboard knight_attacks[64];
// void initKnightAttacks();
Bitboard getKnightAttacks(Square square);

extern const Bitboard king_attacks[64];
// void initKingAttacks();
Bitboard getKingAttacks(Square square);

// extern const Bitboard white_pawn_attacks[64];

// void initWhitePawnAttacks();
extern const Bitboard white_pawn_attacks[64];
Bitboard getWhitePawnAttacks(Square square);
Bitboard getBlackPawnAttacks(Square square);
extern const Bitboard black_pawn_attacks[64];