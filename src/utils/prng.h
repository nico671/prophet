#ifndef PRNG_H
#define PRNG_H

#include <stdint.h>
// adapted from https://www.chessprogramming.org/Bob_Jenkins#RKISS
typedef struct
{
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
} ranctx;
// Rotation macro for 64-bit bits
#define rot(x, k) (((x) << (k)) | ((x) >> (64 - (k))))
void raninit(ranctx* x, uint64_t seed);
uint64_t ranval(ranctx* x);
#endif // PRNG_H