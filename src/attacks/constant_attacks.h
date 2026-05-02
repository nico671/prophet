#ifndef CONSTANT_ATTACKS_H
#define CONSTANT_ATTACKS_H

#include "core/bitboard.h"

/**
 * @brief Precomputed knight attack bitboards for each square, indexing same as Square enum
 *
 */
extern const Bitboard knight_attacks_table[64];

/**
 * @brief Returns the attack bitboard for a knight on the given square
 * @param square The square for which to get the knight attacks (0-63 corresponding to A1-H8)
 * @return Bitboard
 */
Bitboard get_knight_attack_bitboard(Square square);

/**
 * @brief Precomputed king attack bitboards for each square, indexing same as Square enum
 *
 */
extern const Bitboard king_attacks_table[64];

/**
 * @brief Returns the attack bitboard for a king on the given square
 *
 * @param square The square for which to get the king attacks (0-63 corresponding to A1-H8)
 * @return Bitboard
 */
Bitboard get_king_attack_bitboard(Square square);

/**
 * @brief Precomputed white pawn attack bitboards for each square, indexing same as Square enum
 *
 */
extern const Bitboard white_pawn_attacks_table[64];
/**
 * @brief Returns the attack bitboard for a pawn of the given color on the given square
 *
 * @param square The square for which to get the pawn attacks (0-63 corresponding to A1-H8)
 * @param color The color of the pawn (WHITE or BLACK)
 * @return Bitboard
 */
Bitboard get_pawn_attack_bitboard(Square square, Color color);

/**
 * @brief Precomputed black pawn attack bitboards for each square, indexing same as Square enum
 *
 */
extern const Bitboard black_pawn_attacks_table[64];

#endif