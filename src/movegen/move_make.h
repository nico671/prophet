#ifndef MOVE_MAKE_H
#define MOVE_MAKE_H

#include "board/undo.h"
#include "movegen/move.h"

typedef struct CBoard CBoard;

// Main move making and unmaking functions
UndoInfo makeMove(CBoard *board, Move move);
void unmakeMove(CBoard *board, Move move, UndoInfo undoInfo);

// Individual move type functions
UndoInfo makeQuietMove(CBoard *board, Move move);
UndoInfo makeCaptureMove(CBoard *board, Move move);
UndoInfo makeDoublePawnPushMove(CBoard *board, Move move);
UndoInfo makeEnPassantMove(CBoard *board, Move move);
UndoInfo makePromotionMove(CBoard *board, Move move);
UndoInfo makeCastlingMove(CBoard *board, Move move);

#endif // MOVE_MAKE_H
