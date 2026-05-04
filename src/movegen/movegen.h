
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <stdbool.h>

#include "core/chess_types.h"
#include "movegen/move.h"

// Forward declaration to keep this header lightweight.
typedef struct CBoard CBoard;

void gen_all_pseudolegal_moves(CBoard* board, MoveList* move_list);
void generate_capture_moves(CBoard* board, MoveList* out);
void init_move_list(MoveList* move_list);

bool is_square_attacked(CBoard* board, Square square, Color attacker_color);
bool is_king_in_check(CBoard* board, Color side);
void generate_legal_moves(CBoard* board, MoveList* out);

#endif // MOVEGEN_H
