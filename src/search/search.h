#ifndef SEARCH_H
#define SEARCH_H
#include "movegen/move.h"
#include "engine/engine.h"
#include "uci/uci.h"

typedef struct SearchLimits
{
    bool ponder;
    bool infiniteSearch;
    int timeForWhiteMs;
    int timeForBlackMs;
    int incrementForWhiteMs;
    int incrementForBlackMs;
    int movesUntilNextTimeControl;
    int searchDepthLimit;
    int searchNodeLimit;
    int searchForMateInNMoves;
    int searchMoveTimeLimitMs;
    MoveList searchMoves;
} SearchLimits;

// The payload we send to the search thread
typedef struct
{
    CBoard board;        // A COPY of the board, safe from UCI mutations
    SearchLimits limits; // The parsed go parameters
} SearchThreadData;

typedef struct
{
    Move move;
    int score;
} ScoredMove;

// killer moves storage
#define MAX_KILLER_MOVES 2
#define MAX_PLY 64
extern Move killerMoves[MAX_PLY][MAX_KILLER_MOVES]; // [depth][idx] where 0 is newest killer, 1 is previous killer

extern int historyHeuristic[2][64][64]; // [color][from][to]

void *search_worker(void *arg);

void searchOnGoCommand(UCIState *state, SearchLimits goCmd);

void scoreMoves(CBoard *board, MoveList *moveList, ScoredMove *scoredMoves, Move ttMove, int ply);
int quiescence(CBoard *node, int alpha, int beta, int ply);
int negamax(CBoard *node, int depth, int alpha, int beta, Color color, int ply);
void pickNextBestMove(ScoredMove *scoredMoves, int start, int count);
#endif // SEARCH_H