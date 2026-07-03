#ifndef PROPHET_HCEVAL_H
#define PROPHET_HCEVAL_H

#include "board/cboard.h"

// Base piece values used for simple material counting or move
// ordering

#define HC_PAWN_VALUE 100
#define HC_KNIGHT_VALUE 300
#define HC_BISHOP_VALUE 325
#define HC_ROOK_VALUE 500
#define HC_QUEEN_VALUE 900
#define HC_KING_VALUE \
    10000 // Extremely high value to ensure the engine always
          // prioritizes king safety

/**
 * @brief Initializes the Hand-Crafted Evaluation (HCE) module.
 * * Precomputes necessary piece-square tables (PSQTs), combining base
 * piece values with square-centric positional bonuses. This avoids
 * doing redundant arithmetic during the search phase.
 * * @note This function is idempotent (safe to call multiple times)
 * but MUST be called at least once before `hc_evaluate_cboard` is
 * invoked.
 */
void hc_eval_init(void);

/**
 * @brief Statically evaluates a chess board position from the
 * perspective of the side to move.
 * * Uses a "Tapered Evaluation" based on the PeSTO evaluation
 * function. It calculates two separate scores (Midgame and Endgame)
 * based on piece placement, and then interpolates between them based
 * on the current "game phase" (how much non-pawn material is left on
 * the board).
 *
 * @param board Pointer to the board state to evaluate.
 * @return int The centipawn evaluation score.
 * Side to move handled by search.c, so this function always returns a
 * score from the perspective of the side to move (positive = good for
 * side to move, negative = bad for side to move).
 */
int hc_evaluate_cboard(const CBoard* board);

#endif // HCEVAL_H