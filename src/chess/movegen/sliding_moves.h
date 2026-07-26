#ifndef PROPHET_SLIDING_MOVES_H
#define PROPHET_SLIDING_MOVES_H

#include "chess/board/cboard.h"
#include "chess/movegen/move.h"
/** @brief Appends pseudo-legal bishop moves. */
void gen_all_pseudolegal_bishop_moves(CBoard* board, MoveList* move_list);

/** @brief Appends pseudo-legal rook moves. */
void gen_all_pseudolegal_rook_moves(CBoard* board, MoveList* move_list);

/** @brief Appends pseudo-legal queen moves. */
void gen_all_pseudolegal_queen_moves(CBoard* board, MoveList* move_list);

#endif // PROPHET_SLIDING_MOVES_H
