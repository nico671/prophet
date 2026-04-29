#ifndef SLIDING_MOVES_H
#define SLIDING_MOVES_H

#include "movegen/move.h"

typedef struct CBoard CBoard;
void genAllPseudoLegalBishopMoves(CBoard* board, MoveList* moveList);
void genAllPseudoLegalRookMoves(CBoard* board, MoveList* moveList);
void genAllPseudoLegalQueenMoves(CBoard* board, MoveList* moveList);

#endif // SLIDING_MOVES_H
