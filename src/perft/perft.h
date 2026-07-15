#ifndef PROPHET_PERFT_CORE_H
#define PROPHET_PERFT_CORE_H

#include "board/cboard.h"

#include <stdint.h>

uint64_t perft(CBoard* board, int depth);
uint64_t divide(CBoard* board, int depth);

typedef struct {
    const char* name;
    const char* fen;
    uint64_t*   expected_nodes;
    int         max_depth;
} PerftTest;

void run_perft_test_suite(void);
#endif // PERFT_CORE_H
