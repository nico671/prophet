// c
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "cboard.h"
#ifndef SRC_FEN_H
#define SRC_FEN_H

CBoard fenToCBoard(char *fenString);
char *CBoardToFen(CBoard *board);

#endif /* SRC_FEN_H */