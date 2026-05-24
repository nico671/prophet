#include "nnue.h"
#include "nnue_config.h"
#include <arm_neon.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
// Static allocations for the network
alignas(32) static int16_t fc1_w[NNUE_L1_SIZE][NNUE_INPUT_SIZE];
alignas(32) static int16_t fc1_b[NNUE_L1_SIZE];
alignas(32) static int8_t fc2_w[NNUE_L2_SIZE][NNUE_L1_SIZE];
alignas(32) static int32_t fc2_b[NNUE_L2_SIZE];
alignas(32) static int32_t fc3_w[1][NNUE_L2_SIZE];
alignas(32) static int32_t fc3_b[1];

static bool nnue_weights_initialized = false;

#if NNUE_WEIGHT_SHIFT >= 0
#define NNUE_SCALE_DOWN(value) ((value) >> NNUE_WEIGHT_SHIFT)
#else
#define NNUE_SCALE_DOWN(value) ((value) / NNUE_WEIGHT_SCALE)
#endif

void nnue_init(const char* filepath)
{
    if (nnue_weights_initialized) {
        return;
    }
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        printf("Failed to load NNUE weights from %s\n", filepath);
        return;
    }
    fread(fc1_w, sizeof(int16_t), NNUE_L1_SIZE * NNUE_INPUT_SIZE, f);
    fread(fc1_b, sizeof(int16_t), NNUE_L1_SIZE, f);
    fread(fc2_w, sizeof(int8_t), NNUE_L2_SIZE * NNUE_L1_SIZE, f);
    fread(fc2_b, sizeof(int32_t), NNUE_L2_SIZE, f);
    fread(fc3_w, sizeof(int32_t), NNUE_L2_SIZE, f);
    fread(fc3_b, sizeof(int32_t), 1, f);
    fclose(f);
    nnue_weights_initialized = true;
}

static int get_active_features(const CBoard* board, int* features)
{
    int count = 0;
    int stm = board->side_to_move;

    for (int c = 0; c < 2; c++) {
        // base_idx based on python implementation: 0-5 for 'my pieces', 6-11 for 'enemy pieces'
        int is_my_piece = (c == stm);
        int base_idx = is_my_piece ? 0 : 6;

        for (int pt = PAWN; pt <= KING; pt++) {
            Bitboard bb = board->piece_bbs[c][pt];
            while (!bitboard_is_empty(bb)) {
                Square sq = bitboard_lsb_index_unsafe(bb);

                // Flip the square index if it is black's turn
                int sq_idx = (stm == WHITE) ? sq : (sq ^ 56);

                // Subtract 1 from pt to match python's 0-5 indexing
                features[count++] = ((pt - 1) + base_idx) * 64 + sq_idx;
                bitboard_clear_square_bit(&bb, sq);
            }
        }
    }
    return count;
}

static int nnue_feature_index(PieceType piece, Color piece_color, Square sq, Color perspective)
{
    int is_my_piece = (piece_color == perspective);
    int base_idx = is_my_piece ? 0 : 6;
    int sq_idx = (perspective == WHITE) ? sq : (sq ^ 56);
    return ((piece - 1) + base_idx) * 64 + sq_idx;
}

static void nnue_accumulator_add_feature(int16_t* accum, int feature_index, int sign)
{
    // for (int i = 0; i < NNUE_L1_SIZE; i++) {
    //     accum[i] += (int16_t)(sign * fc1_w[i][feature_index]);
    // }

    // point directly to contiguous array of 256 weights for this feature
    const int16_t* w = fc1_w[feature_index];

    // branch outside the loop so the CPU pipeline stays clean and can use SIMD instructions
    if (sign > 0) {
        for (int i = 0; i < NNUE_L1_SIZE; i += 8) {
            int16x8_t w_vec = vld1q_s16(&w[i]);
            int16x8_t accum_vec = vld1q_s16(&accum[i]);
            accum_vec = vaddq_s16(accum_vec, w_vec);
            vst1q_s16(&accum[i], accum_vec);
        }
    } else {
        for (int i = 0; i < NNUE_L1_SIZE; i += 8) {
            int16x8_t w_vec = vld1q_s16(&w[i]);
            int16x8_t accum_vec = vld1q_s16(&accum[i]);
            accum_vec = vsubq_s16(accum_vec, w_vec);
            vst1q_s16(&accum[i], accum_vec);
        }
    }
}

void nnue_accumulator_refresh(const CBoard* board, NnueAccumulator* acc, Color perspective)
{
    if (!acc || !board) {
        return;
    }

    for (int i = 0; i < NNUE_L1_SIZE; i++) {
        acc->values[perspective][i] = fc1_b[i];
    }

    for (int c = 0; c < 2; c++) {
        for (int pt = PAWN; pt <= KING; pt++) {
            Bitboard bb = board->piece_bbs[c][pt];
            while (!bitboard_is_empty(bb)) {
                Square sq = bitboard_lsb_index_unsafe(bb);
                int feature = nnue_feature_index((PieceType)pt, (Color)c, sq, perspective);
                nnue_accumulator_add_feature(acc->values[perspective], feature, 1);
                bitboard_clear_square_bit(&bb, sq);
            }
        }
    }

    acc->valid[perspective] = true;
}

void nnue_accumulator_refresh_both(const CBoard* board, NnueAccumulator* acc)
{
    if (!acc) {
        return;
    }
    nnue_accumulator_refresh(board, acc, WHITE);
    nnue_accumulator_refresh(board, acc, BLACK);
}

void nnue_accumulator_copy(const NnueAccumulator* src, NnueAccumulator* dst)
{
    if (!src || !dst) {
        return;
    }
    for (int p = 0; p < 2; p++) {
        dst->valid[p] = src->valid[p];
        for (int i = 0; i < NNUE_L1_SIZE; i++) {
            dst->values[p][i] = src->values[p][i];
        }
    }
}

static void nnue_accumulator_update_piece(NnueAccumulator* acc, Color piece_color, PieceType piece,
    Square square, int sign)
{
    if (!acc || piece == NO_PIECE || square == NO_SQUARE) {
        return;
    }
    for (int perspective = 0; perspective < 2; perspective++) {
        if (!acc->valid[perspective]) {
            continue;
        }
        int feature = nnue_feature_index(piece, piece_color, square, (Color)perspective);
        nnue_accumulator_add_feature(acc->values[perspective], feature, sign);
    }
}

void nnue_accumulator_apply_move(const CBoard* board, Move move, const NnueAccumulator* prev, NnueAccumulator* next)
{
    if (!board || !prev || !next) {
        return;
    }

    nnue_accumulator_copy(prev, next);

    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);
    Color moving_color = board->side_to_move;
    Color captured_color = (Color)(1 - moving_color);

    PieceType moving_piece = cboard_get_piece_at_square(board, from);

    if (move_is_castling(move)) {
        if (moving_color == WHITE) {
            if (to == G1) {
                nnue_accumulator_update_piece(next, WHITE, KING, E1, -1);
                nnue_accumulator_update_piece(next, WHITE, KING, G1, 1);
                nnue_accumulator_update_piece(next, WHITE, ROOK, H1, -1);
                nnue_accumulator_update_piece(next, WHITE, ROOK, F1, 1);
            } else {
                nnue_accumulator_update_piece(next, WHITE, KING, E1, -1);
                nnue_accumulator_update_piece(next, WHITE, KING, C1, 1);
                nnue_accumulator_update_piece(next, WHITE, ROOK, A1, -1);
                nnue_accumulator_update_piece(next, WHITE, ROOK, D1, 1);
            }
        } else {
            if (to == G8) {
                nnue_accumulator_update_piece(next, BLACK, KING, E8, -1);
                nnue_accumulator_update_piece(next, BLACK, KING, G8, 1);
                nnue_accumulator_update_piece(next, BLACK, ROOK, H8, -1);
                nnue_accumulator_update_piece(next, BLACK, ROOK, F8, 1);
            } else {
                nnue_accumulator_update_piece(next, BLACK, KING, E8, -1);
                nnue_accumulator_update_piece(next, BLACK, KING, C8, 1);
                nnue_accumulator_update_piece(next, BLACK, ROOK, A8, -1);
                nnue_accumulator_update_piece(next, BLACK, ROOK, D8, 1);
            }
        }
        return;
    }

    if (move_is_enpassant(move)) {
        Square captured_pawn_square = to + (8 * (2 * moving_color - 1));
        nnue_accumulator_update_piece(next, moving_color, PAWN, from, -1);
        nnue_accumulator_update_piece(next, moving_color, PAWN, to, 1);
        nnue_accumulator_update_piece(next, captured_color, PAWN, captured_pawn_square, -1);
        return;
    }

    if (move_is_promotion(move)) {
        PieceType promo_piece = move_get_promotion_piecetype(move);
        bool is_capture = bitboard_is_bit_set(board->occupancy_bbs[captured_color], to);
        if (is_capture) {
            PieceType captured_piece = cboard_get_piece_at_square(board, to);
            nnue_accumulator_update_piece(next, captured_color, captured_piece, to, -1);
        }
        nnue_accumulator_update_piece(next, moving_color, PAWN, from, -1);
        nnue_accumulator_update_piece(next, moving_color, promo_piece, to, 1);
        return;
    }

    if (moving_piece != NO_PIECE) {
        nnue_accumulator_update_piece(next, moving_color, moving_piece, from, -1);
        nnue_accumulator_update_piece(next, moving_color, moving_piece, to, 1);
    }

    if (bitboard_is_bit_set(board->occupancy_bbs[captured_color], to)) {
        PieceType captured_piece = cboard_get_piece_at_square(board, to);
        nnue_accumulator_update_piece(next, captured_color, captured_piece, to, -1);
    }
}

static int nnue_evaluate_from_accum(const int16_t* accum)
{
    alignas(32) int8_t out1[NNUE_L1_SIZE];

    int16x8_t zero_vec = vdupq_n_s16(0);
    int16x8_t max_val_vec = vdupq_n_s16(NNUE_FT_SCALE);

    // 1. Clipped ReLU 1 and quantization to int8
    for (int i = 0; i < NNUE_L1_SIZE; i += 8) {
        int16x8_t accum_vec = vld1q_s16(&accum[i]);
        accum_vec = vmaxq_s16(accum_vec, zero_vec); // ReLU
        accum_vec = vminq_s16(accum_vec, max_val_vec); // Clipping
        vst1_s8(&out1[i], vqmovn_s16(accum_vec)); // Narrow to int8_t with saturation
    }

    // 2. Hidden Layer (256 -> 8) with int8 weights and int16 inputs, accumulating into int32 to prevent overflow
    int32_t accum2[NNUE_L2_SIZE];
    for (int i = 0; i < NNUE_L2_SIZE; i++) {
        int32x4_t sum_vec = vdupq_n_s32(fc2_b[i]);

        for (int j = 0; j < NNUE_L1_SIZE; j += 16) {
            int8x16_t v_out = vld1q_s8(&out1[j]);
            int8x16_t v_w = vld1q_s8(&fc2_w[i][j]);
            sum_vec = vdotq_s32(sum_vec, v_w, v_out);
        }

        int32_t sum = vaddvq_s32(sum_vec); // Horizontal add to get the final sum for this neuron
        accum2[i] = sum + fc2_b[i];
    }

    //
    // int8_t out1[NNUE_L1_SIZE];
    // for (int i = 0; i < NNUE_L1_SIZE; i++) {
    //     int16_t val = accum[i];
    //     if (val < 0)
    //         val = 0;
    //     else if (val > NNUE_FT_SCALE)
    //         val = NNUE_FT_SCALE;
    //     out1[i] = (int8_t)val;
    // }

    // // 3. Hidden Layer (8 -> 8)
    // int32_t accum2[NNUE_L2_SIZE];
    // for (int i = 0; i < NNUE_L2_SIZE; i++) {
    //     accum2[i] = fc2_b[i];
    //     for (int j = 0; j < NNUE_L1_SIZE; j++) {
    //         accum2[i] += out1[j] * fc2_w[i][j];
    //     }
    // }

    // 4. Clipped ReLU 2
    int8_t out2[NNUE_L2_SIZE];
    for (int i = 0; i < NNUE_L2_SIZE; i++) {
        int32_t val = NNUE_SCALE_DOWN(accum2[i]); // Scale down to prevent overflow in the next layer
        if (val < 0)
            val = 0;
        else if (val > NNUE_FT_SCALE)
            val = NNUE_FT_SCALE;
        out2[i] = (int8_t)val;
    }

    // 5. Output Layer (8 -> 1)
    int32_t output = fc3_b[0];
    for (int j = 0; j < NNUE_L2_SIZE; j++) {
        output += out2[j] * fc3_w[0][j];
    }

    // Return centipawns (output scale applied in exported weights)
    return NNUE_SCALE_DOWN(output);
}

int nnue_evaluate_cboard(const CBoard* board)
{
    int features[32];
    int count = get_active_features(board, features);

    // 1. Feature Transformer (768 -> 8)
    int16_t accum[NNUE_L1_SIZE];
    for (int i = 0; i < NNUE_L1_SIZE; i++) {
        accum[i] = fc1_b[i];
        for (int j = 0; j < count; j++) {
            accum[i] += fc1_w[i][features[j]];
        }
    }

    return nnue_evaluate_from_accum(accum);
}

int nnue_evaluate_with_accumulator(const CBoard* board, const NnueAccumulator* acc)
{
    if (!board || !acc) {
        return nnue_evaluate_cboard(board);
    }

    Color perspective = board->side_to_move;
    if (!acc->valid[perspective]) {
        NnueAccumulator temp = { 0 };
        nnue_accumulator_refresh(board, &temp, perspective);
        return nnue_evaluate_from_accum(temp.values[perspective]);
    }

    return nnue_evaluate_from_accum(acc->values[perspective]);
}
