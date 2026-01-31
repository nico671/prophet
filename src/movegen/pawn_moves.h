#ifndef PAWN_MOVES_H
#define PAWN_MOVES_H

#include "board/cboard.h"
#include "movegen/movegen.h"

void genSinglePawnPushes(CBoard *board, MoveList *moveList);
void genDoublePawnPushes(CBoard *board, MoveList *moveList);
void genPawnCaptures(CBoard *board, MoveList *moveList);
void genPawnPromotions(CBoard *board, MoveList *moveList);
void genEnPassantPawnMoves(CBoard *board, MoveList *moveList);
void genAllPseudoLegalPawnMoves(CBoard *board, MoveList *moveList);

#endif // PAWN_MOVES_H
