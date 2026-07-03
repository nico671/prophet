#ifndef PROPHET_MOVE_MAKE_H
#define PROPHET_MOVE_MAKE_H

#include "board/cboard.h"
#include "movegen/move.h"

/**
 * @brief Makes the given move on the board, updating all relevant
 * state and returning an UndoInfo struct that can be used to unmake
 * the move later.
 *
 * @param board The current game board.
 * @param move The move to make.
 * @return UndoInfo
 */
UndoInfo make_move(CBoard* board, Move move);

/**
 * @brief Unmakes the given move on the board, restoring all relevant
 * state using the provided UndoInfo.
 *
 * @param board The current game board.
 * @param move The move to unmake (must be the same move that was
 * made).
 * @param undo_info The UndoInfo returned by make_move when the move
 * was made.
 */
void unmake_move(CBoard* board, Move move, UndoInfo undo_info);

#endif // MOVE_MAKE_H
