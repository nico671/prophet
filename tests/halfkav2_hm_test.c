#include "chess/board/cboard.h"
#include "engine/eval/nnue_features.h"
#include "generated/halfkav2_hm_v1_vectors.h"

#include <stdio.h>
#include <string.h>

static int check_perspective(const NnueContractFixture* fixture, Color perspective,
                             const uint16_t* expected, size_t expected_count)
{
    CBoard board;
    if (!fen_string_to_cboard(fixture->fen, &board)) {
        fprintf(stderr, "failed to parse fixture '%s'\n", fixture->name);
        return 1;
    }

    uint16_t actual[NNUE_MAX_ACTIVE_FEATURES];
    size_t actual_count = 0;
    if (!nnue_generate_features(&board, NNUE_FEATURE_HALFKAV2_HM_V1, perspective, actual,
                                NNUE_MAX_ACTIVE_FEATURES, &actual_count)) {
        fprintf(stderr, "feature generation failed for '%s' perspective %d\n", fixture->name,
                perspective);
        return 1;
    }

    if (actual_count != expected_count) {
        fprintf(stderr, "count mismatch for '%s' perspective %d: got %zu, expected %zu\n",
                fixture->name, perspective, actual_count, expected_count);
        return 1;
    }

    for (size_t index = 0; index < expected_count; index++) {
        if (actual[index] != expected[index]) {
            fprintf(stderr,
                    "feature mismatch for '%s' perspective %d at %zu: got %u, expected %u\n",
                    fixture->name, perspective, index, actual[index], expected[index]);
            return 1;
        }
    }

    return 0;
}

static int check_invalid_fens(void)
{
    static const char* invalid_fens[] = {
        "7k/8/8/8/8/8/8/8 w - - 0 1",
        "7k/8/8/8/8/8/8/P6K w - - 0 1",
        "7k/8/8/8/8/8/8/X6K w - - 0 1",
        "7k/8/8/8/8/8/8/K7 w - - 0 0",
    };

    for (size_t index = 0; index < sizeof(invalid_fens) / sizeof(invalid_fens[0]); index++) {
        CBoard board;
        if (fen_string_to_cboard(invalid_fens[index], &board)) {
            fprintf(stderr, "invalid FEN was accepted: %s\n", invalid_fens[index]);
            return 1;
        }
    }
    return 0;
}

static int check_api_rejections(void)
{
    CBoard board;
    if (!fen_string_to_cboard(NNUE_CONTRACT_FIXTURES[0].fen, &board)) {
        fprintf(stderr, "failed to parse the API rejection fixture\n");
        return 1;
    }

    uint16_t output[NNUE_MAX_ACTIVE_FEATURES];
    size_t count = 0;
    if (nnue_generate_features(&board, (NnueFeatureSet)99, WHITE, output, NNUE_MAX_ACTIVE_FEATURES,
                               &count)
        || nnue_generate_features(&board, NNUE_FEATURE_HALFKAV2_HM_V1, WHITE, output, 1, &count)
        || nnue_generate_features(&board, NNUE_FEATURE_HALFKAV2_HM_V1, WHITE, NULL,
                                  NNUE_MAX_ACTIVE_FEATURES, &count)
        || nnue_generate_features(&board, NNUE_FEATURE_HALFKAV2_HM_V1, WHITE, output,
                                  NNUE_MAX_ACTIVE_FEATURES, NULL)) {
        fprintf(stderr, "invalid NNUE feature API input was accepted\n");
        return 1;
    }

    return 0;
}

int main(void)
{
    if (strlen(NNUE_CONTRACT_SHA256) != 64) {
        fprintf(stderr, "invalid contract SHA-256 in generated artifact\n");
        return 1;
    }

    for (size_t index = 0; index < NNUE_CONTRACT_FIXTURE_COUNT; index++) {
        const NnueContractFixture* fixture = &NNUE_CONTRACT_FIXTURES[index];
        if (check_perspective(fixture, WHITE, fixture->white_features, fixture->white_count)
            || check_perspective(fixture, BLACK, fixture->black_features, fixture->black_count)) {
            return 1;
        }
    }

    if (check_invalid_fens()) {
        return 1;
    }
    if (check_api_rejections()) {
        return 1;
    }

    printf("NNUE feature contract passed: %zu fixtures, sha256 %s\n",
           (size_t)NNUE_CONTRACT_FIXTURE_COUNT, NNUE_CONTRACT_SHA256);
    return 0;
}
