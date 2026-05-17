
#include <stdio.h>

#include "board/cboard.h"
#include "engine/engine.h"
#include "eval/hceval.h"
#include "uci/uci.h"
#include <stdio.h>
#include <stdlib.h>
// Assume these are populated via your fread() logic from the .bin file
float fc1_weights[8][768];
float fc1_biases[8];
float fc2_weights[8][8];
float fc2_biases[8];
float fc3_weights[1][8];
float fc3_biases[1];

// Your custom ClippedReLU capped at 1.0
float clipped_relu(float x)
{
    if (x < 0.0f)
        return 0.0f;
    if (x > 1.0f)
        return 1.0f;
    return x;
}

void load_nnue_weights(const char* filepath)
{
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        printf("Error: Could not open NNUE file.\n");
        exit(1);
    }

    // Read directly into the memory addresses of your arrays
    fread(fc1_weights, sizeof(float), 8 * 768, file);
    fread(fc1_biases, sizeof(float), 8, file);

    fread(fc2_weights, sizeof(float), 8 * 8, file);
    fread(fc2_biases, sizeof(float), 8, file);

    fread(fc3_weights, sizeof(float), 1 * 8, file);
    fread(fc3_biases, sizeof(float), 1, file);

    fclose(file);
}

//
void parse_fen_to_features(const char* fen, float* features)
{
    CBoard board = { 0 };
    if (!fen_string_to_cboard(fen, &board)) {
        fprintf(stderr, "Error parsing FEN: %s\n", fen);
        return;
    }
    Color side_to_move = board.side_to_move;
    for (int sq = 0; sq < 64; sq++) {
        PieceType piece_type = board.piece_at_square[sq];
        if (piece_type != NO_PIECE) {
            int piece_type_down = piece_type - 1; // Convert to 0-5 range for indexing
            bool is_my_piece = bitboard_is_bit_set(board.piece_bbs[side_to_move][piece_type], sq);

            int base_idx = piece_type_down + (is_my_piece ? 0 : 6); // 0-5 for own pieces, 6-11 for opponent pieces

            int sq_idx;
            if (side_to_move == WHITE) {
                sq_idx = sq; // White sees the board as is
            } else {
                sq_idx = sq ^ 0x38; // Black sees the board flipped vertically (xor with 56)
            }
            features[base_idx * 64 + sq_idx] = 1.0f;
        }
    }
}

void verify_c(const char* fen)
{
    float input_features[768] = { 0.0f };

    // 1. Parse the FEN into the 768 array.
    // CRITICAL: This logic must exactly mirror your Python Dataset logic!
    parse_fen_to_features(fen, input_features);
    // Print the active feature indices
    printf("Active Features (C Engine): ");
    for (int i = 0; i < 768; i++) {
        if (input_features[i] == 1.0f) {
            printf("%d ", i);
        }
    }
    printf("\n");
    float fc1_out[8] = { 0.0f };
    float fc2_out[8] = { 0.0f };
    float fc3_out = 0.0f;

    // 2. Forward Pass: FC1 (768 -> 8) + ClippedReLU
    for (int i = 0; i < 8; i++) {
        fc1_out[i] = fc1_biases[i];
        for (int j = 0; j < 768; j++) {
            // Note: In an unoptimized dense pass, we multiply by lots of zeros here.
            // This is perfectly fine for verification!
            fc1_out[i] += fc1_weights[i][j] * input_features[j];
        }
        fc1_out[i] = clipped_relu(fc1_out[i]);
    }

    // 3. Forward Pass: FC2 (8 -> 8) + ClippedReLU
    for (int i = 0; i < 8; i++) {
        fc2_out[i] = fc2_biases[i];
        for (int j = 0; j < 8; j++) {
            fc2_out[i] += fc2_weights[i][j] * fc1_out[j];
        }
        fc2_out[i] = clipped_relu(fc2_out[i]);
    }

    // 4. Forward Pass: FC3 (8 -> 1)
    fc3_out = fc3_biases[0];
    for (int j = 0; j < 8; j++) {
        fc3_out += fc3_weights[0][j] * fc2_out[j];
    }

    // 5. Print the final logit
    printf("Reality (C Engine): %.6f\n", fc3_out);
}

int main(void)
{
    load_nnue_weights("/Users/nicocarbone/Documents/dev/prophet-nnue/outs/all_piece_featureset/nnue_weights.bin");
    verify_c("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    return 0;
}
