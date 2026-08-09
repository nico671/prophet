#ifndef PROPHET_ENGINE_H
#define PROPHET_ENGINE_H
#include "chess/board/cboard.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct SearchLimits SearchLimits;

/**
 * @brief Initializes tables, evaluation state, and the transposition table.
 *
 * Safe to call more than once; call before search or move generation.
 */
void engine_init(void);

/**
 * @brief Stops any active search thread and frees all resources
 * associated with the engine.
 */
void engine_shutdown(void);

/**
 * @brief Set the engine position from a FEN string.
 */
bool engine_set_position_fen(const char* fen);

/**
 * @brief Apply a UCI move string to the engine position if it is
 * legal.
 */
bool engine_apply_uci_move(const char* move_str, char* error_buf, size_t error_buf_size);

/**
 * @brief Reset engine state for a new game (board, TT, heuristics).
 */
void engine_new_game(void);

/**
 * @brief Start a search for the current position with the provided
 * limits.
 */
bool engine_start_search(const SearchLimits* limits, char* error_buf, size_t error_buf_size);

/**
 * @brief Stop any active search thread.
 */
void engine_stop_search(void);

/**
 * @brief Handle a ponderhit event from the GUI.
 */
void engine_handle_ponder_hit(void);

/**
 * @brief Resize the transposition table. Returns the applied size
 * (clamped).
 *
 */
bool engine_set_hash_mb(long requested_mb, long* applied_mb);

/**
 * @brief Clear the transposition table.
 */
void engine_clear_hash(void);

/**
 * @brief Print the current engine board state.
 */
void engine_print_board(void);

/**
 * @brief Copy the current engine board state into the provided output
 * struct.
 */
bool engine_copy_board(CBoard* out_board);

/**
 * @brief Enable or disable debug mode, which causes the engine to
 * print additional information about its internal state and search
 * process to the console.
 *
 * @param enabled Whether to enable debug mode.
 */
void engine_set_debug_mode(bool enabled);

/**
 * @brief Check if debug mode is currently enabled.
 *
 * @return true if debug mode is enabled, false otherwise.
 */
bool engine_is_debug_mode(void);

#endif // ENGINE_H
