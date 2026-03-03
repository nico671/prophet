
#ifndef PERFT_H
#define PERFT_H

#include "board/cboard.h"

typedef struct
{
    const char *name;
    const char *fen;
    uint64_t *expected_nodes;
    int max_depth;
} PerftTest;
uint64_t perft(CBoard *board, int depth);
uint64_t divide(CBoard *board, int depth);
#endif // PERFT_H