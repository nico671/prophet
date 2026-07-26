
#ifndef PROPHET_MOVEGEN_H
#define PROPHET_MOVEGEN_H

#include "chess/core/chess_types.h"
#include "chess/movegen/move.h"

#include <stdbool.h>

// Forward declaration to keep this header lightweight.
#include "chess/board/cboard.h"

void init_move_list(MoveList* move_list);
void gen_all_pseudolegal_moves(CBoard* board, MoveList* move_list);
void generate_capture_moves(CBoard* board, MoveList* out);
bool is_king_in_check(CBoard* board, Color side);
void generate_legal_moves(CBoard* board, MoveList* out);

#endif // MOVEGEN_H
