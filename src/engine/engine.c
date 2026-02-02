#include "engine/engine.h"

#include "attacks/sliding_attacks.h"
#include "board/zobrist.h"

void engine_init(void)
{
    // Both of these are made idempotent.
    initSlidingAttacks();
    initZobristKeys();
}
