#ifndef UCI_H
#define UCI_H
#include "board/cboard.h"
#include "movegen/move.h"
#include <pthread.h>

// UCI state structure to track initialization, readiness, quitting, and the current board position. Also manages the search thread and whether a search is active.
typedef struct UCIState
{
    bool initialized;
    bool debugMode;
    bool ready;
    bool quitting;

    CBoard board; // The "root" board managed by UCI

    // Thread management
    pthread_t searchThread;
    bool isSearching; // Tracks if the thread is currently active
} UCIState;

void uciLoop(void);

#endif // UCI_H