#ifndef NNUE_H
#define NNUE_H

#include "board/cboard.h"

void nnue_init(const char* filepath);
int nnue_evaluate_cboard(const CBoard* board);

#endif