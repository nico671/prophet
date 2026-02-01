#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <stdint.h>
#include "utils/prng.h"
#include "core/bitboard.h"
#include "cboard.h"
// 0-5: White Pawn, Knight, Bishop, Rook, Queen, King
// 6-11: Black Pawn, Knight, Bishop, Rook, Queen, King
uint64_t piece_keys[12][64];

uint64_t side_key;

//
uint64_t castle_keys[16];

// 8 possible files for En Passant (or 1 for "None")
uint64_t en_passant_keys[8];

#endif // ZOBRIST_H