#ifndef KING_MOVES_H
#define KING_MOVES_H

#include "board/cboard.h"
#include "movegen/movegen.h"

void genAllPseudoLegalKingNonCastlingMoves(CBoard *board, MoveList *moveList);
void genAllPseudoLegalKingMoves(CBoard *board, MoveList *moveList);

#endif // KING_MOVES_H
