
#ifndef PERFT_H
#define PERFT_H

#include "movegen/movegen.h"
#include "board/fen.h"
#include "attacks/sliding_attacks.h"
uint64_t perft(CBoard *board, int depth);
uint64_t divide(CBoard *board, int depth);
#endif // PERFT_H