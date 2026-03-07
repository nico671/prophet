#include "engine/engine.h"

#include "attacks/sliding_attacks.h"
#include "board/zobrist.h"
#include "eval/hceval.h"
#include "search/tt.h"

void initEngine(void)
{
    // all of these functions are idempotent
    initSlidingAttacks();
    initZobristKeys();
    hcEvalInit();
    initTT(64); // 64 MB TT by default, can be overridden by UCI option later
}
