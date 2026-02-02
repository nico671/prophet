#ifndef PAWN_MOVES_H
#define PAWN_MOVES_H

#include "movegen/move.h"

typedef struct CBoard CBoard;

void genSinglePawnPushes(CBoard *board, MoveList *moveList);
void genDoublePawnPushes(CBoard *board, MoveList *moveList);
void genPawnCaptures(CBoard *board, MoveList *moveList);
void genPawnPromotions(CBoard *board, MoveList *moveList);
void genEnPassantPawnMoves(CBoard *board, MoveList *moveList);
void genAllPseudoLegalPawnMoves(CBoard *board, MoveList *moveList);

#endif // PAWN_MOVES_H
