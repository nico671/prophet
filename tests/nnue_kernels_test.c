#include "engine/eval/nnue_kernels.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#if defined(PROPHET_NNUE_FORCE_SCALAR) \
    && (defined(PROPHET_NNUE_FORCE_NEON) || defined(PROPHET_NNUE_FORCE_DOTPROD))
#error "focused kernel validation cannot force two backends"
#elif defined(PROPHET_NNUE_FORCE_NEON) && defined(PROPHET_NNUE_FORCE_DOTPROD)
#error "focused kernel validation cannot force two backends"
#elif defined(PROPHET_NNUE_FORCE_SCALAR)
#define EXPECTED_BACKEND "scalar"
#elif defined(PROPHET_NNUE_FORCE_NEON)
#define EXPECTED_BACKEND "neon"
#elif defined(PROPHET_NNUE_FORCE_DOTPROD)
#define EXPECTED_BACKEND "dotprod"
#else
#error "focused kernel validation must force a backend"
#endif

static int check_copy(void)
{
    int16_t source[NNUE_KERNEL_VECTOR_SIZE];
    int32_t destination[NNUE_KERNEL_VECTOR_SIZE];
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index++) {
        source[index]      = (int16_t)((int)(index % 9) - 4);
        destination[index] = -1;
    }
    source[0]   = INT16_MIN;
    source[127] = 0;
    source[255] = INT16_MAX;
    nnue_kernel_copy_i16_to_i32(source, destination);
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index++) {
        if (destination[index] != source[index]) {
            return 1;
        }
    }
    return 0;
}

static int check_add_subtract(void)
{
    int16_t row[NNUE_KERNEL_VECTOR_SIZE];
    int32_t accumulator[NNUE_KERNEL_VECTOR_SIZE];
    int32_t original[NNUE_KERNEL_VECTOR_SIZE];
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index++) {
        row[index]         = (int16_t)((int)(index % 5) - 2);
        accumulator[index] = (int32_t)index - 100;
        original[index]    = accumulator[index];
    }
    nnue_kernel_add_i16_to_i32(accumulator, row);
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index++) {
        if (accumulator[index] != original[index] + row[index]) {
            return 1;
        }
    }
    nnue_kernel_subtract_i16_from_i32(accumulator, row);
    return memcmp(accumulator, original, sizeof(accumulator)) != 0;
}

static int check_clamp(void)
{
    int16_t values[NNUE_KERNEL_VECTOR_SIZE] = { 0 };
    uint8_t output[NNUE_KERNEL_VECTOR_SIZE];
    values[0] = INT16_MIN;
    values[1] = -1;
    values[2] = 0;
    values[3] = 1;
    values[4] = 126;
    values[5] = 127;
    values[6] = 128;
    values[7] = INT16_MAX;
    nnue_kernel_clamp_i16_to_u8(values, output);
    static const uint8_t expected[] = { 0, 0, 0, 1, 126, 127, 127, 127 };
    return memcmp(output, expected, sizeof(expected)) != 0;
}

static int check_dot_products(void)
{
    uint8_t inputs[512];
    int8_t weights[512];
    int64_t expected = 0;
    for (size_t index = 0; index < sizeof(inputs); index++) {
        inputs[index]  = (uint8_t)((index * 37U + 11U) % 128U);
        weights[index] = (int8_t)((int)(index % 127U) - 63);
    }
    weights[0] = INT8_MIN;
    weights[1] = INT8_MAX;
    for (size_t index = 0; index < 32; index++) {
        expected += (int64_t)inputs[index] * weights[index];
    }
    if (nnue_kernel_dot_u8_i8(inputs, weights, 32) != expected) {
        return 1;
    }

    expected = 0;
    for (size_t index = 0; index < sizeof(inputs); index++) {
        expected += (int64_t)inputs[index] * weights[index];
    }
    if (nnue_kernel_dot_u8_i8(inputs, weights, sizeof(inputs)) != expected) {
        return 1;
    }

    expected = 0;
    for (size_t index = 0; index < 37; index++) {
        expected += (int64_t)inputs[index] * weights[index];
    }
    if (nnue_kernel_dot_u8_i8(inputs, weights, 37) != expected) {
        return 1;
    }
    return 0;
}

int main(void)
{
    if (strcmp(nnue_kernel_backend(), EXPECTED_BACKEND) || check_copy() || check_add_subtract()
        || check_clamp() || check_dot_products()) {
        fprintf(stderr, "%s NNUE kernel vector failure\n", EXPECTED_BACKEND);
        return 1;
    }
    printf("NNUE %s kernel test passed (backend: %s)\n", EXPECTED_BACKEND,
           nnue_kernel_backend());
    return 0;
}
