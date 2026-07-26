#ifndef PROPHET_HCEVAL_H
#define PROPHET_HCEVAL_H

#include "chess/board/cboard.h"

// Base piece values used for simple material counting or move
// ordering

#define HC_PAWN_VALUE 100
#define HC_KNIGHT_VALUE 300
#define HC_BISHOP_VALUE 325
#define HC_ROOK_VALUE 500
#define HC_QUEEN_VALUE 900
#define HC_KING_VALUE                                                                    \
    10000 // Extremely high value to ensure the engine always
          // prioritizes king safety

/**
 * @brief Initializes the hand-crafted evaluation tables.
 *
 * Safe to call repeatedly; call before hc_evaluate_cboard().
 */
void hc_eval_init(void);

/**
 * @brief Returns a tapered centipawn evaluation for the side to move.
 */
int hc_evaluate_cboard(const CBoard* board);

#endif // HCEVAL_H
