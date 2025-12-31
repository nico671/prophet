
#ifndef PERFT_H
#define PERFT_H

#include "movegen.h"
#include "fen.h"
uint64_t perft(CBoard *board, int depth);
uint64_t divide(CBoard *board, int depth);
#endif // PERFT_H