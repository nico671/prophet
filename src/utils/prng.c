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

/**
 * Generates the next pseudo-random 64-bit number.
 * This implements the three-rotate version (7, 13, 37).
 */
uint64_t ranval(ranctx *x)
{
    uint64_t e = x->a - rot(x->b, 7);
    x->a = x->b ^ rot(x->c, 13);
    x->b = x->c + rot(x->d, 37);
    x->c = x->d + e;
    x->d = e + x->a;
    return x->d;
}

/**
 * Initializes the PRNG state based on a seed.
 * It runs 20 "warm-up" cycles to ensure the bits are well-mixed.
 */
void raninit(ranctx *x, uint64_t seed)
{
    x->a = 0xf1ea5eedULL; // Constant initialization value
    x->b = x->c = x->d = seed;
    for (int i = 0; i < 20; ++i)
    {
        (void)ranval(x);
    }
}