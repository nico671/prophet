#ifndef KNIGHT_MOVES_H
#define KNIGHT_MOVES_H

#include "movegen/move.h"

typedef struct CBoard CBoard;

void genAllPseudoLegalKnightMoves(CBoard *board, MoveList *moveList);

#endif // KNIGHT_MOVES_H
