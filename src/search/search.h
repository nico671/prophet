#ifndef SEARCH_H
#define SEARCH_H

#include "core/chess_types.h"

typedef struct CBoard CBoard;

int negamax(CBoard *node, int depth, int alpha, int beta, Color color);
#endif // SEARCH_H