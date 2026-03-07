#ifndef UCI_H
#define UCI_H
#include "board/cboard.h"
#include "movegen/move.h"
#include <pthread.h>

// Updated UCI state
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