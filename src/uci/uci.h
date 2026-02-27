#ifndef UCI_H
#define UCI_H
#include "board/cboard.h"
#include "movegen/move.h"

typedef struct UCIState
{
    bool initialized; // Whether the engine has been initialized (after "uci" command)
    bool debugMode;   // Whether debug mode is enabled (set by "debug" command)
    bool ready;       // Whether the engine is ready to receive commands (after "isready")
    bool quitting;    // Whether the engine is in the process of quitting (after "quit" command)
    CBoard board;     // Current board state
} UCIState;

void uciLoop(void);

#endif // UCI_H