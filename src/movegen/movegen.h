
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <stdbool.h>

#include "core/chess_types.h"
#include "movegen/move.h"

// Forward declaration to keep this header lightweight.
typedef struct CBoard CBoard;

void genAllPseudoLegalMoves(CBoard *board, MoveList *moveList);
void generateCaptureMoves(CBoard *board, MoveList *out);
void initMoveList(MoveList *moveList);

bool isSquareAttacked(CBoard *board, Square square, Color attackerColor);
bool isKingInCheck(CBoard *board, Color side);
void generateLegalMoves(CBoard *board, MoveList *out);

#endif // MOVEGEN_H
