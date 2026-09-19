#include "engine/eval/nnue_kernels.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#if !defined(PROPHET_NNUE_FORCE_SCALAR)
#error "focused kernel validation must force the scalar backend"
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
    static const uint8_t inputs[] = { 0, 1, 2, 127, 64, 3, 127, 5 };
    static const int8_t weights[] = { -128, -3, 4, -2, 1, 127, -1, 2 };
    if (nnue_kernel_dot_u8_i8(inputs, weights, sizeof(inputs)) != 79) {
        return 1;
    }

    uint8_t dense_inputs[NNUE_KERNEL_VECTOR_SIZE];
    int8_t dense_weights[NNUE_KERNEL_VECTOR_SIZE];
    int64_t expected = 0;
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index++) {
        dense_inputs[index]  = (uint8_t)(index % 128);
        dense_weights[index] = (int8_t)((int)(index % 7) - 3);
        expected += (int64_t)dense_inputs[index] * dense_weights[index];
    }
    return nnue_kernel_dot_u8_i8(dense_inputs, dense_weights, NNUE_KERNEL_VECTOR_SIZE) != expected;
}

int main(void)
{
    if (strcmp(nnue_kernel_backend(), "scalar") || check_copy() || check_add_subtract()
        || check_clamp() || check_dot_products()) {
        fprintf(stderr, "scalar NNUE kernel vector failure\n");
        return 1;
    }
    printf("NNUE scalar kernel test passed (backend: %s)\n", nnue_kernel_backend());
    return 0;
}
