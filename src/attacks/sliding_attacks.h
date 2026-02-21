
#ifndef SLIDING_ATTACKS_H
#define SLIDING_ATTACKS_H
#include "core/bitboard.h"

/**
 * @brief Precomputed rook attack bitboards for each square and occupancy mask variation, indexed by magic bitboard hashing
 *
 */
extern Bitboard rook_attacks[64][4096];
/**
 * @brief Precomputed bishop attack bitboards for each square and occupancy mask variation, indexed by magic bitboard hashing
 *
 */
extern Bitboard bishop_attacks[64][512];

/**
 * @brief Initialize the sliding attack tables for rooks and bishops. This function must be called before using getRookAttacks or getBishopAttacks.
 * @note This function is safe to call concurrently from multiple threads. It uses
 * an atomic initialization state machine (0 = uninitialized, 1 = initializing,
 * 2 = initialized) to ensure that the attack tables are materialized exactly
 * once, even when multiple threads attempt to initialize them at the same time.
 * The first thread that successfully transitions the state from 0 to 1
 * performs the full initialization of the rook and bishop attack tables.
 * Other threads that call this function while initialization is in progress
 * will spin until the state becomes 2, at which point the tables are fully
 * initialized and ready for use.
 * After initialization has completed (state == 2), subsequent calls return
 * immediately without performing any additional work.
 */
void initSlidingAttacks(void);

/**
 * @brief Fast lookup functions for rook attacks using magic bitboard indexing.
 *
 * @param square The square of the rook (0-63 corresponding to A1-H8)
 * @param occupancy The occupancy bitboard of the current position, which will be masked and transformed to index into the attack table
 * @return Bitboard
 */
Bitboard getRookAttacks(Square square, Bitboard occupancy);
/**
 * @brief Fast lookup functions for bishop attacks using magic bitboard indexing.
 *
 * @param square The square of the bishop (0-63 corresponding to A1-H8)
 * @param occupancy The occupancy bitboard of the current position, which will be masked and transformed to index into the attack table
 * @return Bitboard
 */
Bitboard getBishopAttacks(Square square, Bitboard occupancy);

/**
 * @brief Fast lookup function for queen attacks, which combines rook and bishop attacks.
 * @note This function computes the queen attacks by performing a bitwise OR of the rook and bishop attacks for the given square and occupancy. It is not a separate entry in the attack tables, since queen attacks can be derived from the existing rook and bishop tables.
 * @param square The square of the queen (0-63 corresponding to A1-H8)
 * @param occupancy The occupancy bitboard of the current position, which will be masked and transformed to index into the rook and bishop attack tables
 * @return Bitboard
 */
Bitboard getQueenAttacks(Square square, Bitboard occupancy);

/**
 * @brief Internal function used by the attack table initializer to generate the attack bitboard for a bishop on a given square with a specific blocker configuration. This function is not used directly in move generation, but is essential for populating the attack tables during initialization.
 *
 * @param square The square of the bishop (0-63 corresponding to A1-H8)
 * @param blockers The occupancy bitboard representing the positions of blockers for this test case, which will be used to calculate the attack bitboard for this specific configuration
 * @return Bitboard
 */
Bitboard generateBishopAttacks(Square square, Bitboard blockers);

/**
 * @brief Internal function used by the attack table initializer to generate the attack bitboard for a rook on a given square with a specific blocker configuration. This function is not used directly in move generation, but is essential for populating the attack tables during initialization.
 *
 * @param square The square of the rook (0-63 corresponding to A1-H8)
 * @param blockers The occupancy bitboard representing the positions of blockers for this test case, which will be used to calculate the attack bitboard for this specific configuration
 * @return Bitboard
 */
Bitboard generateRookAttacks(Square square, Bitboard blockers);

#endif