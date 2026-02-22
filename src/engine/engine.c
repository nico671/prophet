#include "engine/engine.h"

#include "attacks/sliding_attacks.h"
#include "board/zobrist.h"
#include "hcevaluation/hceval.h"

void engine_init(void)
{
    // all of these functions are idempotent
    initSlidingAttacks();
    initZobristKeys();
    hcEvalInit();
}
