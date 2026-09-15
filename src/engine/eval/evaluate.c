#include "engine/eval/evaluate.h"

#include "engine/eval/hceval.h"

static bool use_nnue;

void evaluate_set_use_nnue(bool enabled)
{
    use_nnue = enabled;
}

bool evaluate_uses_nnue(void)
{
    return use_nnue;
}

int evaluate_cboard(const CBoard* board, const NnueAccumulator* accumulator)
{
    if (use_nnue) {
        if (accumulator) {
            return nnue_evaluate_accumulator(board, accumulator);
        }
        return nnue_evaluate_cboard(board);
    }
    return hc_evaluate_cboard(board);
}
