#ifndef SLIDING_MOVES_H
#define SLIDING_MOVES_H

#include "board/cboard.h"
#include "movegen/move.h"
void gen_all_pseudolegal_bishop_moves(CBoard* board, MoveList* move_list);
void gen_all_pseudolegal_rook_moves(CBoard* board, MoveList* move_list);
void gen_all_pseudolegal_queen_moves(CBoard* board, MoveList* move_list);

#endif // SLIDING_MOVES_H
