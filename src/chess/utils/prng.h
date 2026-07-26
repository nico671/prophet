#ifndef PROPHET_PRNG_H
#define PROPHET_PRNG_H

#include <stdint.h>
// adapted from https://www.chessprogramming.org/Bob_Jenkins#RKISS
typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
} ranctx;
// Rotation macro for 64-bit bits
#define rot(x, k) (((x) << (k)) | ((x) >> (64 - (k))))
/** @brief Seeds a Bob Jenkins RKISS generator. */
void raninit(ranctx* x, uint64_t seed);

/** @brief Returns the next value from a seeded RKISS generator. */
uint64_t ranval(ranctx* x);
#endif // PRNG_H
