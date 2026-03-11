#ifndef CONSTANT_MOVES_H
#define CONSTANT_MOVES_H

#include "movegen/move.h"

typedef struct CBoard CBoard;

void genAllPseudoLegalKingNonCastlingMoves(CBoard *board, MoveList *moveList);
void genAllPseudoLegalKingCastlingMoves(CBoard *board, MoveList *moveList);
void genAllPseudoLegalKingMoves(CBoard *board, MoveList *moveList);

void genAllPseudoLegalKnightMoves(CBoard *board, MoveList *moveList);

void genSinglePawnPushes(CBoard *board, MoveList *moveList);
void genDoublePawnPushes(CBoard *board, MoveList *moveList);
void genPawnCaptures(CBoard *board, MoveList *moveList);
void genPawnPromotions(CBoard *board, MoveList *moveList);
void genEnPassantPawnMoves(CBoard *board, MoveList *moveList);
void genAllPseudoLegalPawnMoves(CBoard *board, MoveList *moveList);
#endif // CONSTANT_MOVES_H