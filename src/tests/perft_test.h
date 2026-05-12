
#ifndef PERFT_TEST_H
#define PERFT_TEST_H

#include "board/cboard.h"

typedef struct
{
    const char* name;
    const char* fen;
    uint64_t* expected_nodes;
    int max_depth;
} PerftTest;
#endif // PERFT_TEST_H