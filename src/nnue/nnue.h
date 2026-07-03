#ifndef PROPHET_NNUE_H
#define PROPHET_NNUE_H

#include "board/cboard.h"

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

#endif