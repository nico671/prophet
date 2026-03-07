#ifndef ENGINE_H
#define ENGINE_H
#include "board/cboard.h"
#include "movegen/move.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

// Global, thread-safe flag to interrupt the search
extern atomic_bool engine_stop_search;

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

// Initializes global engine state (attack tables, zobrist keys, eval function helpers).
// Safe to call multiple times.
void initEngine(void);

#endif // ENGINE_H
