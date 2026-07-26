#ifndef PROPHET_PERFT_CORE_H
#define PROPHET_PERFT_CORE_H

#include "chess/board/cboard.h"

#include <stdint.h>

/** @brief Counts legal move-tree nodes to @p depth without changing @p board. */
uint64_t perft(CBoard* board, int depth);

/** @brief Prints and counts per-root-move perft totals. */
uint64_t divide(CBoard* board, int depth);

typedef struct {
    const char* name;
    const char* fen;
    uint64_t* expected_nodes;
    int max_depth;
} PerftTest;

/** @brief Runs the built-in perft regression positions. */
void run_perft_test_suite(void);
#endif // PERFT_CORE_H
