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

// TODO: call this in engine initialization or if evaluation mode switched to nnue?
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
        // base_idx matches Python: 0-5 for 'my pieces', 6-11 for 'enemy pieces'
        int is_my_piece = (c == stm);
        int base_idx = is_my_piece ? 0 : 6;

        for (int pt = PAWN; pt <= KING; pt++) {
            Bitboard bb = board->piece_bbs[c][pt];
            while (!bitboard_is_empty(bb)) {
                Square sq = bitboard_lsb_index_unsafe(bb);

                // Flip the square index if it is black's turn
                int sq_idx = (stm == WHITE) ? sq : (sq ^ 56);

                // FIX: Subtract 1 from pt to match python's 0-5 indexing
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

    // 4. Clipped ReLU 2 (Using bitwise shift)
    int8_t out2[8];
    for (int i = 0; i < 8; i++) {
        int32_t val = accum2[i] >> 6; // Fast division by 64
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

// float fc1_weights[8][768];
// float fc1_biases[8];
// float fc2_weights[8][8];
// float fc2_biases[8];
// float fc3_weights[1][8];
// float fc3_biases[1];

// // Your custom ClippedReLU capped at 1.0
// float clipped_relu(float x)
// {
//     if (x < 0.0f)
//         return 0.0f;
//     if (x > 1.0f)
//         return 1.0f;
//     return x;
// }

// void load_nnue_weights(const char* filepath)
// {
//     FILE* file = fopen(filepath, "rb");
//     if (!file) {
//         printf("Error: Could not open NNUE file.\n");
//         exit(1);
//     }

//     // Read directly into the memory addresses of your arrays
//     fread(fc1_weights, sizeof(float), 8 * 768, file);
//     fread(fc1_biases, sizeof(float), 8, file);

//     fread(fc2_weights, sizeof(float), 8 * 8, file);
//     fread(fc2_biases, sizeof(float), 8, file);

//     fread(fc3_weights, sizeof(float), 1 * 8, file);
//     fread(fc3_biases, sizeof(float), 1, file);

//     fclose(file);
// }

// //
// void parse_fen_to_features(const char* fen, float* features)
// {
//     CBoard board = { 0 };
//     if (!fen_string_to_cboard(fen, &board)) {
//         fprintf(stderr, "Error parsing FEN: %s\n", fen);
//         return;
//     }
//     Color side_to_move = board.side_to_move;
//     for (int sq = 0; sq < 64; sq++) {
//         PieceType piece_type = board.piece_at_square[sq];
//         if (piece_type != NO_PIECE) {
//             int piece_type_down = piece_type - 1; // Convert to 0-5 range for indexing
//             bool is_my_piece = bitboard_is_bit_set(board.piece_bbs[side_to_move][piece_type], sq);

//             int base_idx = piece_type_down + (is_my_piece ? 0 : 6); // 0-5 for own pieces, 6-11 for opponent pieces

//             int sq_idx;
//             if (side_to_move == WHITE) {
//                 sq_idx = sq; // White sees the board as is
//             } else {
//                 sq_idx = sq ^ 0x38; // Black sees the board flipped vertically (xor with 56)
//             }
//             features[base_idx * 64 + sq_idx] = 1.0f;
//         }
//     }
// }

// void verify_c(const char* fen)
// {
//     float input_features[768] = { 0.0f };

//     // 1. Parse the FEN into the 768 array.
//     // CRITICAL: This logic must exactly mirror your Python Dataset logic!
//     parse_fen_to_features(fen, input_features);
//     // Print the active feature indices
//     printf("Active Features (C Engine): ");
//     for (int i = 0; i < 768; i++) {
//         if (input_features[i] == 1.0f) {
//             printf("%d ", i);
//         }
//     }
//     printf("\n");
//     float fc1_out[8] = { 0.0f };
//     float fc2_out[8] = { 0.0f };
//     float fc3_out = 0.0f;

//     // 2. Forward Pass: FC1 (768 -> 8) + ClippedReLU
//     for (int i = 0; i < 8; i++) {
//         fc1_out[i] = fc1_biases[i];
//         for (int j = 0; j < 768; j++) {
//             // Note: In an unoptimized dense pass, we multiply by lots of zeros here.
//             // This is perfectly fine for verification!
//             fc1_out[i] += fc1_weights[i][j] * input_features[j];
//         }
//         fc1_out[i] = clipped_relu(fc1_out[i]);
//     }

//     // 3. Forward Pass: FC2 (8 -> 8) + ClippedReLU
//     for (int i = 0; i < 8; i++) {
//         fc2_out[i] = fc2_biases[i];
//         for (int j = 0; j < 8; j++) {
//             fc2_out[i] += fc2_weights[i][j] * fc1_out[j];
//         }
//         fc2_out[i] = clipped_relu(fc2_out[i]);
//     }

//     // 4. Forward Pass: FC3 (8 -> 1)
//     fc3_out = fc3_biases[0];
//     for (int j = 0; j < 8; j++) {
//         fc3_out += fc3_weights[0][j] * fc2_out[j];
//     }

//     // 5. Print the final logit
//     printf("Reality (C Engine): %.6f\n", fc3_out);
// }