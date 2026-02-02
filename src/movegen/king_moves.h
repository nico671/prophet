#ifndef KING_MOVES_H
#define KING_MOVES_H

#include "movegen/move.h"

typedef struct CBoard CBoard;

void genAllPseudoLegalKingNonCastlingMoves(CBoard *board, MoveList *moveList);
void genAllPseudoLegalKingMoves(CBoard *board, MoveList *moveList);

#endif // KING_MOVES_H
