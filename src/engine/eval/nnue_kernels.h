#ifndef PROPHET_NNUE_KERNELS_H
#define PROPHET_NNUE_KERNELS_H

#include <stddef.h>
#include <stdint.h>

#define NNUE_KERNEL_VECTOR_SIZE 256

void nnue_kernel_copy_i16_to_i32(const int16_t source[NNUE_KERNEL_VECTOR_SIZE],
                                 int32_t destination[NNUE_KERNEL_VECTOR_SIZE]);
void nnue_kernel_add_i16_to_i32(int32_t accumulator[NNUE_KERNEL_VECTOR_SIZE],
                                const int16_t row[NNUE_KERNEL_VECTOR_SIZE]);
void nnue_kernel_subtract_i16_from_i32(int32_t accumulator[NNUE_KERNEL_VECTOR_SIZE],
                                       const int16_t row[NNUE_KERNEL_VECTOR_SIZE]);
void nnue_kernel_clamp_i16_to_u8(const int16_t values[NNUE_KERNEL_VECTOR_SIZE],
                                 uint8_t output[NNUE_KERNEL_VECTOR_SIZE]);
int64_t nnue_kernel_dot_u8_i8(const uint8_t* input, const int8_t* weights, size_t count);
const char* nnue_kernel_backend(void);

#endif // PROPHET_NNUE_KERNELS_H
