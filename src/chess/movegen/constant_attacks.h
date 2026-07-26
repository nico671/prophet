#ifndef PROPHET_CONSTANT_ATTACKS_H
#define PROPHET_CONSTANT_ATTACKS_H

#include "chess/core/bitboard.h"

/**
 * @brief Precomputed knight attack bitboards for each square, indexed
 * by Square
 *
 */
extern const Bitboard knight_attacks_table[64];

/**
 * @brief Returns the attack bitboard for a knight on the given square
 * @param square The square for which to get the knight attacks (0-63
 * corresponding to A1-H8)
 * @return Bitboard
 */
static inline Bitboard get_knight_attack_bitboard(Square square)
{
    return knight_attacks_table[square];
}

/**
 * @brief Precomputed king attack bitboards for each square, indexed
 * by Square
 *
 */
extern const Bitboard king_attacks_table[64];

/**
 * @brief Returns the attack bitboard for a king on the given square
 *
 * @param square The square for which to get the king attacks (0-63
 * corresponding to A1-H8)
 * @return Bitboard
 */
static inline Bitboard get_king_attack_bitboard(Square square)
{
    return king_attacks_table[square];
}

/**
 * @brief Precomputed pawn attack bitboards for each square,
 * indexed [color][square]
 */
extern const Bitboard pawn_attacks_table[2][64];

/**
 * @brief Returns the attack bitboard for a pawn of the given color on
 * the given square
 *
 * @param square The square for which to get the pawn attacks (0-63
 * corresponding to A1-H8)
 * @param color The color of the pawn (WHITE or BLACK)
 * @return Bitboard
 */
static inline Bitboard get_pawn_attack_bitboard(Square square, Color color)
{
    return pawn_attacks_table[color][square];
}

#endif
