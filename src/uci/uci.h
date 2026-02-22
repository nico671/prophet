#ifndef UCI_H
#define UCI_H
#include "board/cboard.h"
#include "movegen/move.h"

Move parseMoveLAN(const char *moveStr, const CBoard *board);

#endif // UCI_H