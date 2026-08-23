#ifndef PROPHET_BENCHMARK_H
#define PROPHET_BENCHMARK_H

#include <stddef.h>

typedef struct {
    const char* fen;
    const char* moves;
} BenchmarkPosition;

const BenchmarkPosition* benchmark_positions(size_t* count);

#endif // PROPHET_BENCHMARK_H
