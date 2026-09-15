#ifndef PROPHET_NNUE_FEATURES_H
#define PROPHET_NNUE_FEATURES_H

#include "chess/board/cboard.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NNUE_FEATURE_COUNT 22528
#define NNUE_MAX_ACTIVE_FEATURES 32

/** Identifies a supported NNUE input feature contract. */
typedef enum {
    NNUE_FEATURE_HALFKAV2_HM_V1 = 0,
} NnueFeatureSet;

/**
 * Generates sorted active feature indices for one king perspective.
 *
 * The output is sorted numerically. The function returns false for an
 * unsupported feature set, an invalid board, or an output buffer that is too
 * small.
 */
bool nnue_generate_features(const CBoard* board, NnueFeatureSet feature_set, Color perspective,
                            uint16_t* output, size_t capacity, size_t* count);

/**
 * Returns one active feature index using the same mapping as the full generator.
 *
 * Returns false when the board does not have a valid NNUE piece placement.
 */
bool nnue_feature_index_for_piece(const CBoard* board, NnueFeatureSet feature_set,
                                  Color perspective, Color color, PieceType piece, Square square,
                                  uint16_t* output);

#endif // PROPHET_NNUE_FEATURES_H
