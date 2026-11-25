#ifndef PROPHET_EVALUATE_H
#define PROPHET_EVALUATE_H

#include "chess/board/cboard.h"
#include "engine/eval/nnue.h"

/** Enables or disables NNUE selection for subsequent evaluations. */
void evaluate_set_use_nnue(bool enabled);

/** Returns true when evaluation selects NNUE instead of HCE. */
bool evaluate_uses_nnue(void);

/** Evaluates board with the selected evaluator and an optional NNUE accumulator. */
int evaluate_cboard(const CBoard* board, const NnueAccumulator* accumulator);

#endif // PROPHET_EVALUATE_H
