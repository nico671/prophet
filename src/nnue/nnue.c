#include "nnue.h"
#include <stdint.h>
#include <stdio.h>

// Static allocations for the network
static int16_t fc1_w[8][768];
static int16_t fc1_b[8];
static int8_t fc2_w[8][8];
static int32_t fc2_b[8];
static int32_t fc3_w[1][8];
static int32_t fc3_b[1];

static bool nnue_weights_initialized = false;

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
    fread(fc1_w, sizeof(int16_t), 8 * 768, f);
    fread(fc1_b, sizeof(int16_t), 8, f);
    fread(fc2_w, sizeof(int8_t), 8 * 8, f);
    fread(fc2_b, sizeof(int32_t), 8, f);
    fread(fc3_w, sizeof(int32_t), 8, f);
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

int nnue_evaluate_cboard(const CBoard* board)
{
    int features[32];
    int count = get_active_features(board, features);

    // 1. Feature Transformer (768 -> 8)
    int16_t accum[8];
    for (int i = 0; i < 8; i++) {
        accum[i] = fc1_b[i];
        for (int j = 0; j < count; j++) {
            accum[i] += fc1_w[i][features[j]];
        }
    }

    // 2. Clipped ReLU 1
    int8_t out1[8];
    for (int i = 0; i < 8; i++) {
        int16_t val = accum[i];
        if (val < 0)
            val = 0;
        else if (val > 127)
            val = 127;
        out1[i] = (int8_t)val;
    }

    // 3. Hidden Layer (8 -> 8)
    int32_t accum2[8];
    for (int i = 0; i < 8; i++) {
        accum2[i] = fc2_b[i];
        for (int j = 0; j < 8; j++) {
            accum2[i] += out1[j] * fc2_w[i][j];
        }
    }

    // 4. Clipped ReLU 2
    int8_t out2[8];
    for (int i = 0; i < 8; i++) {
        int32_t val = accum2[i] >> 6; // Scale down by 64 to prevent overflow in the next layer
        if (val < 0)
            val = 0;
        else if (val > 127)
            val = 127;
        out2[i] = (int8_t)val;
    }

    // 5. Output Layer (8 -> 1)
    int32_t output = fc3_b[0];
    for (int j = 0; j < 8; j++) {
        output += out2[j] * fc3_w[0][j];
    }

    // Return centipawns
    return output >> 6; // Scale back down by 64
}
