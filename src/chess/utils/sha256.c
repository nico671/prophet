#include "sha256.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t state[8];
    uint64_t bit_count;
    uint8_t buffer[64];
    size_t buffer_count;
} Sha256;

static const uint32_t round_constants[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static uint32_t rotate_right(uint32_t value, unsigned amount)
{
    return (value >> amount) | (value << (32U - amount));
}

static uint32_t choose(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (~x & z);
}

static uint32_t majority(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

static void transform(Sha256* ctx, const uint8_t block[64])
{
    uint32_t words[64];
    for (size_t i = 0; i < 16; i++) {
        words[i] = ((uint32_t)block[i * 4] << 24) | ((uint32_t)block[i * 4 + 1] << 16)
            | ((uint32_t)block[i * 4 + 2] << 8) | (uint32_t)block[i * 4 + 3];
    }
    for (size_t i = 16; i < 64; i++) {
        uint32_t s0 = rotate_right(words[i - 15], 7) ^ rotate_right(words[i - 15], 18)
            ^ (words[i - 15] >> 3);
        uint32_t s1 = rotate_right(words[i - 2], 17) ^ rotate_right(words[i - 2], 19)
            ^ (words[i - 2] >> 10);
        words[i] = words[i - 16] + s0 + words[i - 7] + s1;
    }

    uint32_t a = ctx->state[0];
    uint32_t b = ctx->state[1];
    uint32_t c = ctx->state[2];
    uint32_t d = ctx->state[3];
    uint32_t e = ctx->state[4];
    uint32_t f = ctx->state[5];
    uint32_t g = ctx->state[6];
    uint32_t h = ctx->state[7];

    for (size_t i = 0; i < 64; i++) {
        uint32_t s1    = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
        uint32_t temp1 = h + s1 + choose(e, f, g) + round_constants[i] + words[i];
        uint32_t s0    = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
        uint32_t temp2 = s0 + majority(a, b, c);
        h              = g;
        g              = f;
        f              = e;
        e              = d + temp1;
        d              = c;
        c              = b;
        b              = a;
        a              = temp1 + temp2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void initialize(Sha256* ctx)
{
    *ctx = (Sha256) { .state = {
                          0x6a09e667,
                          0xbb67ae85,
                          0x3c6ef372,
                          0xa54ff53a,
                          0x510e527f,
                          0x9b05688c,
                          0x1f83d9ab,
                          0x5be0cd19,
                      } };
}

static void update(Sha256* ctx, const uint8_t* data, size_t size)
{
    ctx->bit_count += (uint64_t)size * 8U;
    while (size > 0) {
        size_t copy_count = 64 - ctx->buffer_count;
        if (copy_count > size) {
            copy_count = size;
        }
        memcpy(ctx->buffer + ctx->buffer_count, data, copy_count);
        ctx->buffer_count += copy_count;
        data += copy_count;
        size -= copy_count;
        if (ctx->buffer_count == 64) {
            transform(ctx, ctx->buffer);
            ctx->buffer_count = 0;
        }
    }
}

static void finalize(Sha256* ctx, uint8_t digest[32])
{
    size_t original_count            = ctx->buffer_count;
    ctx->buffer[ctx->buffer_count++] = 0x80;
    if (ctx->buffer_count > 56) {
        while (ctx->buffer_count < 64) {
            ctx->buffer[ctx->buffer_count++] = 0;
        }
        transform(ctx, ctx->buffer);
        ctx->buffer_count = 0;
    }
    while (ctx->buffer_count < 56) {
        ctx->buffer[ctx->buffer_count++] = 0;
    }
    for (int i = 0; i < 8; i++) {
        ctx->buffer[56 + i] = (uint8_t)(ctx->bit_count >> (56 - i * 8));
    }
    transform(ctx, ctx->buffer);
    ctx->buffer_count = original_count;

    for (size_t i = 0; i < 8; i++) {
        digest[i * 4]     = (uint8_t)(ctx->state[i] >> 24);
        digest[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        digest[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        digest[i * 4 + 3] = (uint8_t)ctx->state[i];
    }
}

bool sha256_file(const char* path, char out[65])
{
    if (!path || !out) {
        return false;
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        return false;
    }

    Sha256 ctx;
    initialize(&ctx);
    uint8_t buffer[4096];
    bool success = true;
    while (1) {
        size_t count = fread(buffer, 1, sizeof(buffer), file);
        if (count > 0) {
            update(&ctx, buffer, count);
        }
        if (count < sizeof(buffer)) {
            if (ferror(file)) {
                success = false;
            }
            break;
        }
    }
    if (fclose(file) != 0) {
        success = false;
    }
    if (!success) {
        return false;
    }

    uint8_t digest[32];
    finalize(&ctx, digest);
    for (size_t i = 0; i < sizeof(digest); i++) {
        (void)snprintf(out + i * 2, 3, "%02x", digest[i]);
    }
    out[64] = '\0';
    return true;
}

bool sha256_bytes(const void* data, size_t size, char out[65])
{
    if (!data || !out) {
        return false;
    }
    Sha256 ctx;
    initialize(&ctx);
    update(&ctx, data, size);
    uint8_t digest[32];
    finalize(&ctx, digest);
    for (size_t i = 0; i < sizeof(digest); i++) {
        (void)snprintf(out + i * 2, 3, "%02x", digest[i]);
    }
    out[64] = '\0';
    return true;
}
