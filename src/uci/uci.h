#ifndef UCI_H
#define UCI_H
#include "board/cboard.h"
#include "movegen/move.h"
#include <pthread.h>

void uciLoop(void);

#endif // UCI_H