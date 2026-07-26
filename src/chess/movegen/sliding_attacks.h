
#ifndef PROPHET_SLIDING_ATTACKS_H
#define PROPHET_SLIDING_ATTACKS_H
#include "chess/core/bitboard.h"
// TODO: look into fancy bitboards
extern const Bitboard rook_occupancy_maps[64];
extern const Bitboard bishop_occupancy_maps[64];
extern const Bitboard RMagic[64];
extern const Bitboard BMagic[64];
extern const int RBits[64];
extern const int BBits[64];

/**
 * @brief Computes the index into the magic bitboard attack table for
 * a given occupancy, magic number, and number of bits.
 * @param occupancy The occupancy bitboard of the current position,
 * which will be masked and transformed to index into the attack table
 * @param magic The magic number for the square
 * @param bits The number of bits to use for the index
 * @return The computed index
 */
static inline int magic_index(Bitboard occupancy, Bitboard magic, int bits)
{
    return (int)((occupancy * magic) >> (64 - bits));
}

/**
 * @brief Precomputed rook attack bitboards for each square and
 * occupancy mask variation, indexed by magic bitboard hashing
 *
 */
extern Bitboard rook_attacks[64][4096];
/**
 * @brief Precomputed bishop attack bitboards for each square and
 * occupancy mask variation, indexed by magic bitboard hashing
 *
 */
extern Bitboard bishop_attacks[64][512];

/**
 * @brief Initializes the rook and bishop attack tables.
 *
 * Safe to call repeatedly and concurrently. Other callers spin until the
 * first initializer finishes.
 */
void init_sliding_attacks(void);

/**
 * @brief Returns rook attacks for a valid square and occupancy.
 */
static inline Bitboard get_rook_attack_bitboard(Square square, Bitboard occupancy)
{
    occupancy &= rook_occupancy_maps[square];
    int index = magic_index(occupancy, RMagic[square], RBits[square]);
    return rook_attacks[square][index];
}

/**
 * @brief Returns bishop attacks for a valid square and occupancy.
 */
static inline Bitboard get_bishop_attack_bitboard(Square square, Bitboard occupancy)
{
    occupancy &= bishop_occupancy_maps[square];
    int index = magic_index(occupancy, BMagic[square], BBits[square]);
    return bishop_attacks[square][index];
}

/**
 * @brief Returns queen attacks by combining rook and bishop attacks.
 */
static inline Bitboard get_queen_attack_bitboard(Square square, Bitboard occupancy)
{
    return get_rook_attack_bitboard(square, occupancy)
        | get_bishop_attack_bitboard(square, occupancy);
}

/**
 * @brief Generates bishop attacks for table initialization.
 */
Bitboard generate_bishop_attacks(Square square, Bitboard blockers);

/**
 * @brief Generates rook attacks for table initialization.
 */
Bitboard generate_rook_attacks(Square square, Bitboard blockers);

#endif
