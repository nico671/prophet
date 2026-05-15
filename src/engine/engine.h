#ifndef ENGINE_H
#define ENGINE_H
#include "board/cboard.h"
#include "movegen/move.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

// Global, thread-safe flag to interrupt the search
extern atomic_bool search_stop_flag;

// Set when the engine is pondering and receives a ponderhit command, so the search thread can react appropriately
extern atomic_bool search_is_pondering;

/**
 * @brief This struct encapsulates all the parameters that can be specified in a UCI "go" command, including time controls, depth limits, node limits, and specific moves to search for.
 *
 * It is used to communicate the search parameters from the main thread (which handles UCI commands) to the search thread that performs the actual search.
 * The search thread will read these limits and use them to determine when to stop searching and how to prioritize moves.
 */
typedef struct SearchLimits {
    bool ponder;
    bool infinite_search;
    int time_for_white_ms;
    int time_for_black_ms;
    int increment_for_white_ms;
    int increment_for_black_ms;
    int moves_until_next_time_control;
    int depth_limit;
    int node_limit;
    int search_for_mate_in_n_moves;
    int time_limit_ms;
    MoveList search_moves;
} SearchLimits;

/**
 * @brief Represents the state of the chess engine.
 *
 * This struct holds the current board position, the search thread handle, and flags for whether a search is active and whether debug mode is enabled.
 * It serves as a central repository for the engine's state that can be accessed and modified by various functions throughout the engine's operation.
 */
typedef struct EngineState {
    CBoard board; // Engine-managed root position
    pthread_t search_thread;
    bool is_searching;
    bool is_debug_mode;
} EngineState;

// The payload we send to the search thread
typedef struct {
    CBoard board; // A COPY of the board, safe from UCI mutations
    SearchLimits search_limits; // The parsed go parameters
} SearchThreadData;

/**
 * @brief Initializes the chess engine, setting up global state and data structures including sliding attack tables, Zobrist hashing keys, evaluation parameters, and the transposition table. This function is idempotent and can be safely called multiple times without adverse effects. It must be called before any search or move generation functions are used to ensure that all necessary data structures are properly initialized.
 */
void engine_init(void);

/**
 * @brief Stops any active search thread and leaves the engine in a clean state.
 */
void engine_shutdown(void);

/**
 * @brief Set the engine position from a FEN string.
 */
bool engine_set_position_fen(const char* fen);

/**
 * @brief Parse a UCI move string into a Move using the current engine position.
 *
 * @note This function generates all legal moves in the current position and checks if any of them match the provided UCI move string.
 * This ensures that the move is not only syntactically valid but also legal in the current position (e.g., not moving a piece that isn't there, not leaving the king in check, etc.).
 *
 * @returns parsed Move if successful, or a Move with NO_SQUARE from/to if the move was invalid or illegal. If the move string is valid but the move is not legal in the current position, an error message will be written to error_buf.
 */
Move engine_parse_and_create_uci_move(const char* move_str, char* error_buf, size_t error_buf_size);

/**
 * @brief Apply a UCI move string to the engine position if it is legal.
 */
bool engine_apply_uci_move(const char* move_str, char* error_buf,
    size_t error_buf_size);

/**
 * @brief Reset engine state for a new game (board, TT, heuristics).
 */
void engine_new_game(void);

/**
 * @brief Start a search for the current position with the provided limits.
 */
bool engine_start_search(const SearchLimits* limits, char* error_buf,
    size_t error_buf_size);

/**
 * @brief Stop any active search thread.
 */
void engine_stop_search(void);

/**
 * @brief Handle a ponderhit event from the GUI.
 */
void engine_handle_ponder_hit(void);

/**
 * @brief Resize the transposition table. Returns the applied size (clamped).
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
 * @brief Copy the current engine board state into the provided output struct.
 */
bool engine_copy_board(CBoard* out_board);

/**
 * @brief Enable or disable debug mode, which causes the engine to print additional information about its internal state and search process to the console.
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
