#include "engine/engine.h"

#include "attacks/sliding_attacks.h"
#include "board/zobrist.h"
#include "eval/hceval.h"
#include "search/tt.h"

void init_engine(void)
{
    // all of these functions are idempotent
    init_sliding_attacks();
    init_zobrist_keys();
    hc_eval_init();
    init_tt(64); // 64 MB TT by default, can be overridden by UCI option later
}
