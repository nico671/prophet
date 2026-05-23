#ifndef NNUE_H
#define NNUE_H

#include "board/cboard.h"
#include "core/chess_types.h"
#include "movegen/move.h"
#include "nnue_config.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Accumulator for the NNUE feature transformer.
 *
 * Stores the pre-activation sums for both perspectives (white/black), with
 * biases already applied. Each perspective matches the feature indexing logic
 * used by the Python AllPieces extractor ("my" vs "enemy" and mirrored squares).
 */
typedef struct {
    int16_t values[2][NNUE_L1_SIZE];
    bool valid[2];
} NnueAccumulator;

/**
 * @brief Initialize the NNUE network with weights from a file
 * @param filepath Path to the file containing the NNUE weights
 *
 * Currently expects (768->8->8->1) and weights quantized in same way
 */
void nnue_init(const char* filepath);

/**
 * @brief Evaluate the position using the NNUE network
 * @param board The current board position
 * @return int The evaluated score in centipawns
 */
int nnue_evaluate_cboard(const CBoard* board);

/**
 * @brief Evaluate the position using a precomputed accumulator.
 * @param board The current board position.
 * @param acc Precomputed accumulator for both perspectives.
 * @return int The evaluated score in centipawns.
 */
int nnue_evaluate_with_accumulator(const CBoard* board, const NnueAccumulator* acc);

/**
 * @brief Refresh accumulator for a single perspective.
 * @param board The current board position.
 * @param acc Accumulator to fill.
 * @param perspective Perspective to build (WHITE/BLACK).
 */
void nnue_accumulator_refresh(const CBoard* board, NnueAccumulator* acc, Color perspective);

/**
 * @brief Refresh accumulator for both perspectives.
 * @param board The current board position.
 * @param acc Accumulator to fill.
 */
void nnue_accumulator_refresh_both(const CBoard* board, NnueAccumulator* acc);

/**
 * @brief Copy accumulator values.
 * @param src Source accumulator.
 * @param dst Destination accumulator.
 */
void nnue_accumulator_copy(const NnueAccumulator* src, NnueAccumulator* dst);

/**
 * @brief Apply a move incrementally to produce the next accumulator.
 * @param board Board state BEFORE making the move.
 * @param move Move to apply.
 * @param prev Accumulator for the current position.
 * @param next Accumulator for the next position (output).
 */
void nnue_accumulator_apply_move(const CBoard* board, Move move, const NnueAccumulator* prev, NnueAccumulator* next);

#endif