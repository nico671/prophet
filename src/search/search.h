#ifndef SEARCH_H
#define SEARCH_H
#include "board/cboard.h"
#include "core/chess_types.h"
#include "movegen/movegen.h"
#include "movegen/move_make.h"
#include "hcevaluation/hceval.h"
int negamax(CBoard *node, int depth, int alpha, int beta, Color color);
#endif // SEARCH_H