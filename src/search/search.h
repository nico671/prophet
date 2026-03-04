#ifndef SEARCH_H
#define SEARCH_H
#include "movegen/move.h"
#include "engine/engine.h"

typedef struct
{
    Move move;
    int score;
} ScoredMove;

void *search_worker(void *arg);

void searchOnGoCommand(UCIState *state, SearchLimits goCmd);

void scoreMoves(CBoard *board, MoveList *moveList, ScoredMove *scoredMoves, Move ttMove);
void sortScoredMoves(ScoredMove *scoredMoves, int count);
int negamax(CBoard *node, int depth, int alpha, int beta, Color color);

#endif // SEARCH_H