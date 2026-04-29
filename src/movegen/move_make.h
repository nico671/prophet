#ifndef MOVE_MAKE_H
#define MOVE_MAKE_H

#include "movegen/move.h"

typedef struct CBoard CBoard;

/**
 * @brief Makes the given move on the board, updating all relevant state and returning an UndoInfo struct that can be used to unmake the move later.
 *
 * @param board The current game board.
 * @param move The move to make.
 * @return UndoInfo
 */
UndoInfo makeMove(CBoard* board, Move move);

/**
 * @brief Unmakes the given move on the board, restoring all relevant state using the provided UndoInfo.
 *
 * @param board The current game board.
 * @param move The move to unmake (must be the same move that was made).
 * @param undoInfo The UndoInfo returned by makeMove when the move was made.
 */
void unmakeMove(CBoard* board, Move move, UndoInfo undoInfo);

UndoInfo makeQuietMove(CBoard* board, Move move);
UndoInfo makeCaptureMove(CBoard* board, Move move);
UndoInfo makeDoublePawnPushMove(CBoard* board, Move move);
UndoInfo makeEnPassantCapture(CBoard* board, Move move);
UndoInfo makePromotionMove(CBoard* board, Move move);
UndoInfo makeCastlingMove(CBoard* board, Move move);

#endif // MOVE_MAKE_H
