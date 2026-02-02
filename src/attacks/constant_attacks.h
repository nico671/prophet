#ifndef CONSTANT_ATTACKS_H
#define CONSTANT_ATTACKS_H

#include "core/bitboard.h"

extern const Bitboard knight_attacks[64];
Bitboard getKnightAttacks(Square square);

extern const Bitboard king_attacks[64];
Bitboard getKingAttacks(Square square);

extern const Bitboard white_pawn_attacks[64];
Bitboard getWhitePawnAttacks(Square square);
Bitboard getBlackPawnAttacks(Square square);
extern const Bitboard black_pawn_attacks[64];

#endif