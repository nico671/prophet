#ifndef PROPHET_NNUE_H
#define PROPHET_NNUE_H

#include "chess/board/cboard.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NNUE_ACCUMULATOR_SIZE 256

typedef struct {
    int16_t values[2][NNUE_ACCUMULATOR_SIZE];
    bool valid[2];
} NnueAccumulator;

/** Loads and fully validates a supported PNUE file before replacing the active network. */
bool nnue_load_file(const char* path, char* error, size_t error_size);

/** Releases the active network, if any. */
void nnue_unload(void);

/** Returns true when a fully validated network is active. */
bool nnue_is_loaded(void);

/** Returns the active network architecture identifier, or an empty string. */
const char* nnue_architecture_id(void);

/** Returns the active network model identifier as lowercase hexadecimal, or an empty string. */
const char* nnue_model_identifier(void);

/** Builds both king-perspective accumulators from a board. */
bool nnue_refresh_accumulator(const CBoard* board, NnueAccumulator* accumulator);

/** Copies and eagerly updates a child accumulator from its parent and child boards. */
bool nnue_update_accumulator(const CBoard* parent, const CBoard* child,
                             const NnueAccumulator* parent_accumulator,
                             NnueAccumulator* child_accumulator);

/** Evaluates an already valid accumulator for board->side_to_move. */
int nnue_evaluate_accumulator(const CBoard* board, const NnueAccumulator* accumulator);

/**
 * Builds a temporary full-refresh accumulator and evaluates it.
 *
 * Search uses its persistent per-ply accumulator instead. This function is
 * the scalar reference implementation and supports direct callers and tests.
 */
int nnue_evaluate_cboard(const CBoard* board);

#endif // PROPHET_NNUE_H
