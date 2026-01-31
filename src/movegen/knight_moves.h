#ifndef KNIGHT_MOVES_H
#define KNIGHT_MOVES_H

#include "board/cboard.h"
#include "movegen/movegen.h"

void genAllPseudoLegalKnightMoves(CBoard *board, MoveList *moveList);

#endif // KNIGHT_MOVES_H
