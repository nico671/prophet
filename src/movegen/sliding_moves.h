#ifndef SLIDING_MOVES_H
#define SLIDING_MOVES_H

#include "board/cboard.h"
#include "movegen/movegen.h"
void genAllPseudoLegalBishopMoves(CBoard *board, MoveList *moveList);
void genAllPseudoLegalRookMoves(CBoard *board, MoveList *moveList);
void genAllPseudoLegalQueenMoves(CBoard *board, MoveList *moveList);

#endif // SLIDING_MOVES_H
