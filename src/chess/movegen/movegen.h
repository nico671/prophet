
#ifndef PROPHET_MOVEGEN_H
#define PROPHET_MOVEGEN_H

#include "chess/core/chess_types.h"
#include "chess/movegen/move.h"

#include <stdbool.h>

// Forward declaration to keep this header lightweight.
#include "chess/board/cboard.h"

/** @brief Resets a move list before adding moves. */
void init_move_list(MoveList* move_list);

/** @brief Appends pseudo-legal moves; callers must filter king safety. */
void gen_all_pseudolegal_moves(CBoard* board, MoveList* move_list);

/** @brief Appends legal captures and promotions to @p out. */
void generate_capture_moves(CBoard* board, MoveList* out);

/** @brief Reports whether @p side's king is attacked. */
bool is_king_in_check(CBoard* board, Color side);

/** @brief Appends moves that leave the side to move out of check. */
void generate_legal_moves(CBoard* board, MoveList* out);

#endif // MOVEGEN_H
