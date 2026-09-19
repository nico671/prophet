#include "engine/eval/nnue_kernels.h"

void nnue_kernel_copy_i16_to_i32(const int16_t source[NNUE_KERNEL_VECTOR_SIZE],
                                 int32_t destination[NNUE_KERNEL_VECTOR_SIZE])
{
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index++) {
        destination[index] = source[index];
    }
}

void nnue_kernel_add_i16_to_i32(int32_t accumulator[NNUE_KERNEL_VECTOR_SIZE],
                                const int16_t row[NNUE_KERNEL_VECTOR_SIZE])
{
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index++) {
        accumulator[index] += row[index];
    }
}

void nnue_kernel_subtract_i16_from_i32(int32_t accumulator[NNUE_KERNEL_VECTOR_SIZE],
                                       const int16_t row[NNUE_KERNEL_VECTOR_SIZE])
{
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index++) {
        accumulator[index] -= row[index];
    }
}

void nnue_kernel_clamp_i16_to_u8(const int16_t values[NNUE_KERNEL_VECTOR_SIZE],
                                 uint8_t output[NNUE_KERNEL_VECTOR_SIZE])
{
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index++) {
        int value     = values[index];
        output[index] = (uint8_t)(value < 0 ? 0 : value > 127 ? 127 : value);
    }
}

int64_t nnue_kernel_dot_u8_i8(const uint8_t* input, const int8_t* weights, size_t count)
{
    int64_t sum = 0;
    for (size_t index = 0; index < count; index++) {
        sum += (int64_t)input[index] * weights[index];
    }
    return sum;
}

const char* nnue_kernel_backend(void)
{
    return "scalar";
}
