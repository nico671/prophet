#include "engine/eval/nnue_kernels.h"

#if defined(PROPHET_NNUE_FORCE_SCALAR) \
    && (defined(PROPHET_NNUE_FORCE_NEON) || defined(PROPHET_NNUE_FORCE_DOTPROD))
#error "NNUE backend selections cannot conflict"
#elif defined(PROPHET_NNUE_FORCE_NEON) && defined(PROPHET_NNUE_FORCE_DOTPROD)
#error "NNUE backend selections cannot conflict"
#endif

#if defined(PROPHET_NNUE_FORCE_DOTPROD)
#if !defined(__aarch64__) || !defined(__ARM_NEON) || !defined(__ARM_FEATURE_DOTPROD)
#error "forced NNUE dot product requires ARM64 NEON dot-product support"
#endif
#define PROPHET_NNUE_USE_NEON    1
#define PROPHET_NNUE_USE_DOTPROD 1
#elif defined(PROPHET_NNUE_FORCE_NEON)
#if !defined(__aarch64__) || !defined(__ARM_NEON)
#error "forced NNUE NEON requires ARM64 NEON"
#endif
#define PROPHET_NNUE_USE_NEON    1
#define PROPHET_NNUE_USE_DOTPROD 0
#elif defined(PROPHET_NNUE_FORCE_SCALAR)
#define PROPHET_NNUE_USE_NEON    0
#define PROPHET_NNUE_USE_DOTPROD 0
#elif defined(__aarch64__) && defined(__ARM_NEON) && defined(__ARM_FEATURE_DOTPROD)
#define PROPHET_NNUE_USE_NEON    1
#define PROPHET_NNUE_USE_DOTPROD 1
#elif defined(__aarch64__) && defined(__ARM_NEON)
#define PROPHET_NNUE_USE_NEON    1
#define PROPHET_NNUE_USE_DOTPROD 0
#else
#define PROPHET_NNUE_USE_NEON    0
#define PROPHET_NNUE_USE_DOTPROD 0
#endif

#if PROPHET_NNUE_USE_NEON
#include <arm_neon.h>
#endif

#if PROPHET_NNUE_USE_NEON

void nnue_kernel_copy_i16_to_i32(const int16_t source[NNUE_KERNEL_VECTOR_SIZE],
                                 int32_t destination[NNUE_KERNEL_VECTOR_SIZE])
{
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index += 8) {
        int16x8_t values = vld1q_s16(source + index);
        vst1q_s32(destination + index, vmovl_s16(vget_low_s16(values)));
        vst1q_s32(destination + index + 4, vmovl_s16(vget_high_s16(values)));
    }
}

void nnue_kernel_add_i16_to_i32(int32_t accumulator[NNUE_KERNEL_VECTOR_SIZE],
                                const int16_t row[NNUE_KERNEL_VECTOR_SIZE])
{
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index += 8) {
        int16x8_t values = vld1q_s16(row + index);
        vst1q_s32(accumulator + index,
                  vaddq_s32(vld1q_s32(accumulator + index), vmovl_s16(vget_low_s16(values))));
        vst1q_s32(accumulator + index + 4,
                  vaddq_s32(vld1q_s32(accumulator + index + 4),
                            vmovl_s16(vget_high_s16(values))));
    }
}

void nnue_kernel_subtract_i16_from_i32(int32_t accumulator[NNUE_KERNEL_VECTOR_SIZE],
                                       const int16_t row[NNUE_KERNEL_VECTOR_SIZE])
{
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index += 8) {
        int16x8_t values = vld1q_s16(row + index);
        vst1q_s32(accumulator + index,
                  vsubq_s32(vld1q_s32(accumulator + index), vmovl_s16(vget_low_s16(values))));
        vst1q_s32(accumulator + index + 4,
                  vsubq_s32(vld1q_s32(accumulator + index + 4),
                            vmovl_s16(vget_high_s16(values))));
    }
}

void nnue_kernel_clamp_i16_to_u8(const int16_t values[NNUE_KERNEL_VECTOR_SIZE],
                                 uint8_t output[NNUE_KERNEL_VECTOR_SIZE])
{
    const int16x8_t lower = vdupq_n_s16(0);
    const int16x8_t upper = vdupq_n_s16(127);
    for (size_t index = 0; index < NNUE_KERNEL_VECTOR_SIZE; index += 8) {
        int16x8_t clamped = vmaxq_s16(vld1q_s16(values + index), lower);
        clamped           = vminq_s16(clamped, upper);
        vst1_u8(output + index, vmovn_u16(vreinterpretq_u16_s16(clamped)));
    }
}

int64_t nnue_kernel_dot_u8_i8(const uint8_t* input, const int8_t* weights, size_t count)
{
#if PROPHET_NNUE_USE_DOTPROD
    int32x4_t sum_vector = vdupq_n_s32(0);
    size_t index         = 0;
    for (; index + 16 <= count; index += 16) {
        int8x16_t inputs_signed = vreinterpretq_s8_u8(vld1q_u8(input + index));
        sum_vector              = vdotq_s32(sum_vector, inputs_signed, vld1q_s8(weights + index));
    }
    int64_t sum = (int64_t)vaddvq_s32(sum_vector);
#else
    int32x4_t sum_low  = vdupq_n_s32(0);
    int32x4_t sum_high = vdupq_n_s32(0);
    size_t index       = 0;
    for (; index + 8 <= count; index += 8) {
        uint8x8_t inputs = vld1_u8(input + index);
        int16x8_t inputs_wide
            = vreinterpretq_s16_u16(vmovl_u8(inputs));
        int16x8_t weights_wide = vmovl_s8(vld1_s8(weights + index));
        sum_low                = vaddq_s32(
            sum_low, vmull_s16(vget_low_s16(inputs_wide), vget_low_s16(weights_wide)));
        sum_high = vaddq_s32(
            sum_high, vmull_s16(vget_high_s16(inputs_wide), vget_high_s16(weights_wide)));
    }
    int64_t sum = (int64_t)vaddvq_s32(sum_low) + vaddvq_s32(sum_high);
#endif
    for (; index < count; index++) {
        sum += (int64_t)input[index] * weights[index];
    }
    return sum;
}

const char* nnue_kernel_backend(void)
{
#if PROPHET_NNUE_USE_DOTPROD
    return "dotprod";
#else
    return "neon";
#endif
}

#else

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

#endif
