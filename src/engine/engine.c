#include "engine/engine.h"

#include "board/zobrist.h"
#include "eval/hceval.h"
#include "movegen/move_make.h"
#include "movegen/movegen.h"
#include "movegen/sliding_attacks.h"
#include "nnue/nnue.h"
#include "search/search.h"
#include "tt/tt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// standard starting position FEN string
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

// atomic flag for controlling search thread shutdown
atomic_bool search_stop_flag = false;

// atomic flag to indicate if the search thread is currently pondering
// (used for ponderhit handling)
atomic_bool search_is_pondering = false;

/**
 * @brief Sets the error message in the provided buffer.
 * @param error_buf The buffer to store the error message.
 * @param error_buf_size The size of the error buffer.
 * @param message The error message to set.
 */
static void set_error(char* error_buf, size_t error_buf_size, const char* message)
{
    if (!error_buf || error_buf_size == 0) {
        return;
    }
    snprintf(error_buf, error_buf_size, "%s", message);
    // flush so that error messages are not delayed (important for
    // GUIs)
    fflush(stderr);
}

// central engine state struct
static EngineState engine_state = { 0 };

void engine_init(void)
{
    // initialization of all global lookup tables and heuristics
    // all functions are idempotent
    init_sliding_attacks();
    init_zobrist_keys();
    // hc_eval_init(); // TODO: make this a config option b/w
    // classical eval and nnue eval
    nnue_init("/Users/nicocarbone/Documents/dev/prophet/artifacts/v0.1.0/"
              "nnue_weights.bin"); // TODO: make path a config option

    // initialize 64 MB TT by default, can be overridden by UCI option
    // later
    init_tt(64);

    // reset search control flags
    atomic_store(&search_stop_flag, false);
    atomic_store(&search_is_pondering, false);

    engine_state.is_searching = false;
    engine_state.is_debug_mode = false;

    // set initial board position to standard starting position, not
    // sure if this is technically uci compliant but hasn't made a
    // difference with GUIs or Lichess
    engine_set_position_fen(START_FEN);
}

void engine_shutdown(void)
{
    // ensure any active search thread is stopped before shutting down
    // the engine and freeing resources
    engine_stop_search();
    free_tt();
}

void engine_stop_search(void)
{
    if (!engine_state.is_searching) {
        return; // no active search to stop
    }

    // signal search thread to stop and wait for it to finish
    atomic_store(&search_stop_flag, true);

    // block the main thread until the search thread actually exists
    // prevents starting a new search until the previous search thread
    // has fully cleaned up and exited
    pthread_join(engine_state.search_thread, NULL);

    engine_state.is_searching = false;
    atomic_store(&search_is_pondering, false);
}

bool engine_set_position_fen(const char* fen)
{
    if (!fen) {
        return false;
    }
    return fen_string_to_cboard(fen, &engine_state.board);
}

bool engine_apply_uci_move(const char* move_str, char* error_buf,
    size_t error_buf_size)
{
    // parse the move and verify legality using the current engine
    // position
    Move parsed_move
        = move_from_uci_string(&engine_state.board, move_str, error_buf,
            error_buf_size);

    if (parsed_move == MOVE_NONE) {
        set_error(error_buf, error_buf_size, "Invalid or illegal move");
        return false;
    }

    make_move(&engine_state.board, parsed_move);
    return true;
}

void engine_new_game(void)
{
    // ensure any active search is stopped before resetting the engine
    // state for a new game
    engine_stop_search();
    // reset to startpos
    engine_set_position_fen(START_FEN);
    // clear TT and heuristics to remove any information from the
    // previous game that could affect the new game
    clear_tt();
}

bool engine_start_search(const SearchLimits* limits, char* error_buf,
    size_t error_buf_size)
{
    if (!limits) {
        set_error(error_buf, error_buf_size, "Search limits not provided");
        return false;
    }

    // if a search is already in progress, stop it before starting a
    // new one with the new limits
    if (engine_state.is_searching) {
        engine_stop_search();
    }

    // reset search control flags before starting the search thread
    atomic_store(&search_stop_flag, false);
    atomic_store(&search_is_pondering, limits->ponder);
    engine_state.is_searching = true;

    // Allocate thread data on the heap. We do this instead of passing
    // a pointer to engine_state directly to prevent the GUI thread
    // from mutating the board while the search thread is reading it.
    SearchThreadData* thread_data = malloc(sizeof(SearchThreadData));
    if (thread_data == NULL) {
        set_error(error_buf, error_buf_size, "memory allocation failed");
        engine_state.is_searching = false;
        return false;
    }

    // create a copy of the current board and search limits for the
    // search thread to use
    thread_data->board = engine_state.board;
    thread_data->search_limits = *limits;

    // dispatch the search_worker function to a new background thread
    // passing the thread data as an argument
    if (pthread_create(&engine_state.search_thread, NULL, search_worker, thread_data)
        != 0) {
        set_error(error_buf, error_buf_size, "failed to create search thread");
        free(thread_data);
        engine_state.is_searching = false;
        return false;
    }

    return true;
}

void engine_handle_ponder_hit(void)
{
    // delegarte to search module which updates time management
    // internal state
    on_ponder_hit();
}

bool engine_set_hash_mb(long requested_mb, long* applied_mb)
{
    // clamp requested size to a reasonable range (1 MB to 1024 MB) to
    // prevent excessive memory usage or invalid sizes
    long mb = requested_mb;
    if (mb < 1) {
        mb = 1;
    } else if (mb > 1024) {
        mb = 1024;
    }
    // We must stop the search before resizing the hash, search should
    // be stopped before resizing the hash but just to be safe
    engine_stop_search();
    init_tt((size_t)mb);

    if (applied_mb) {
        *applied_mb = mb; // report the actual applied size back to
                          // the caller (after clamping)
    }

    return true;
}

void engine_clear_hash(void)
{
    engine_stop_search(); // ensure no active search is using the TT
                          // before clearing it
    clear_tt();
}

void engine_print_board(void) { print_cboard(&engine_state.board); }

void engine_set_debug_mode(bool enabled) { engine_state.is_debug_mode = enabled; }

bool engine_is_debug_mode(void) { return engine_state.is_debug_mode; }

bool engine_copy_board(CBoard* out_board)
{
    if (!out_board) {
        return false;
    }

    *out_board = engine_state.board;
    return true;
}
