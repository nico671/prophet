#include "engine/engine.h"

#include "chess/board/zobrist.h"
#include "chess/movegen/move_make.h"
#include "chess/movegen/movegen.h"
#include "chess/movegen/sliding_attacks.h"
#include "engine/benchmark.h"
#include "engine/eval/hceval.h"
#include "engine/search/search.h"
#include "engine/tt/tt.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

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
    // flush so that error messages are not delayed (important for GUIs)
    fflush(stderr);
}

typedef struct {
    CBoard board;
    pthread_t search_thread;
    SearchInput search_input;
    SearchControl search_control;
    bool is_searching;
    bool is_debug_mode;
    int multipv;
} EngineState;

static EngineState engine_state = { 0 };

static void* search_thread_main(void* arg)
{
    (void)arg;
    (void)search_run(&engine_state.search_input, &engine_state.search_control);
    return NULL;
}

void engine_init(void)
{
    // initialization of all global lookup tables and heuristics
    // all functions are idempotent
    init_sliding_attacks();
    init_zobrist_keys();
    hc_eval_init();

    // initialize 64 MB TT by default, can be overridden by UCI option
    // later
    init_tt(64);

    search_control_reset(&engine_state.search_control, false);

    engine_state.is_searching  = false;
    engine_state.is_debug_mode = false;
    engine_state.multipv       = 1;

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

    search_request_stop(&engine_state.search_control);

    // block the main thread until the search thread actually exists
    // prevents starting a new search until the previous search thread
    // has fully cleaned up and exited
    pthread_join(engine_state.search_thread, NULL);

    engine_state.is_searching = false;
}

bool engine_set_position_fen(const char* fen)
{
    if (!fen) {
        return false;
    }
    return fen_string_to_cboard(fen, &engine_state.board);
}

bool engine_apply_uci_move(const char* move_str, char* error_buf, size_t error_buf_size)
{
    // parse the move and verify legality using the current engine position
    Move parsed_move
        = move_from_uci_string(&engine_state.board, move_str, error_buf, error_buf_size);

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

bool engine_start_search(const SearchLimits* limits, char* error_buf, size_t error_buf_size)
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

    search_control_reset(&engine_state.search_control, limits->ponder);
    engine_state.is_searching                     = true;

    engine_state.search_input.board               = engine_state.board;
    engine_state.search_input.limits              = *limits;
    engine_state.search_input.suppress_uci_output = false;

    if (pthread_create(&engine_state.search_thread, NULL, search_thread_main, NULL) != 0) {
        set_error(error_buf, error_buf_size, "failed to create search thread");
        engine_state.is_searching = false;
        return false;
    }

    return true;
}

bool engine_run_benchmark(int depth, EngineBenchmarkResult* result, char* error_buf,
                          size_t error_buf_size)
{
    if (depth < 1 || depth > 64) {
        set_error(error_buf, error_buf_size, "Benchmark depth must be in [1, 64]");
        return false;
    }

    engine_stop_search();

    CBoard saved_board = engine_state.board;
    size_t position_count;
    const BenchmarkPosition* positions     = benchmark_positions(&position_count);
    EngineBenchmarkResult benchmark_result = { 0 };
    bool success                           = true;
    char setup_error[128]                  = "";

    clear_tt();

    for (size_t i = 0; i < position_count; i++) {
        if (!engine_set_position_fen(positions[i].fen)) {
            set_error(error_buf, error_buf_size, "Failed to parse benchmark position");
            success = false;
            break;
        }

        if (positions[i].moves) {
            const char* move = positions[i].moves;
            while (*move) {
                while (*move == ' ') {
                    move++;
                }
                if (!*move) {
                    break;
                }

                const char* move_end = move;
                while (*move_end && *move_end != ' ') {
                    move_end++;
                }

                size_t move_length = (size_t)(move_end - move);
                if (move_length >= sizeof(setup_error)) {
                    set_error(error_buf, error_buf_size, "Benchmark move is too long");
                    success = false;
                    break;
                }

                char move_string[sizeof(setup_error)];
                memcpy(move_string, move, move_length);
                move_string[move_length] = '\0';
                setup_error[0]           = '\0';
                if (!engine_apply_uci_move(move_string, setup_error, sizeof(setup_error))) {
                    set_error(error_buf, error_buf_size, "Failed to apply benchmark move");
                    success = false;
                    break;
                }
                move = move_end;
            }
            if (!success) {
                break;
            }
        }

        SearchLimits limits = { 0 };
        limits.depth_limit  = depth;
        limits.multipv      = 1;

        SearchControl control;
        search_control_reset(&control, false);
        SearchInput input = {
            .board               = engine_state.board,
            .limits              = limits,
            .suppress_uci_output = true,
        };
        SearchResult search_result = search_run(&input, &control);
        if (!search_result.completed) {
            set_error(error_buf, error_buf_size, "Benchmark search was stopped");
            success = false;
            break;
        }

        benchmark_result.nodes += search_result.nodes;
        benchmark_result.elapsed_ms += search_result.elapsed_ms;
        benchmark_result.positions++;

        uint64_t position_nps = search_result.elapsed_ms > 0
            ? (search_result.nodes * 1000ULL) / (uint64_t)search_result.elapsed_ms
            : 0;
        fprintf(stderr,
                "Bench position %zu/%zu nodes %" PRIu64 " time %" PRId64 " nps %" PRIu64 "\n",
                i + 1, position_count, search_result.nodes, search_result.elapsed_ms,
                position_nps);
        fflush(stderr);
    }

    engine_state.board = saved_board;
    clear_tt();

    if (!success) {
        return false;
    }

    if (result) {
        *result = benchmark_result;
    }
    return true;
}

void engine_handle_ponder_hit(void)
{
    if (engine_state.is_searching) {
        search_handle_ponder_hit(&engine_state.search_control, &engine_state.search_input);
    }
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
    // We must stop the search before resizing the hash, search should be stopped before resizing
    // the hash but just to be safe
    engine_stop_search();
    init_tt((size_t)mb);

    if (applied_mb) {
        *applied_mb = mb; // report the actual applied size back to the caller (after clamping)
    }

    return true;
}

bool engine_set_multipv(int requested_multipv, int* applied_multipv)
{
    int multipv = requested_multipv;
    if (multipv < 1) {
        multipv = 1;
    } else if (multipv > MAX_LEGAL_MOVES) {
        multipv = MAX_LEGAL_MOVES;
    }

    engine_stop_search();
    engine_state.multipv = multipv;

    if (applied_multipv) {
        *applied_multipv = multipv;
    }

    return true;
}

int engine_get_multipv(void)
{
    return engine_state.multipv;
}

void engine_clear_hash(void)
{
    engine_stop_search(); // ensure no active search is using the TT before
                          // clearing it
    clear_tt();
}

void engine_print_board(void)
{
    print_cboard(&engine_state.board);
}

void engine_set_debug_mode(bool enabled)
{
    engine_state.is_debug_mode = enabled;
}

bool engine_is_debug_mode(void)
{
    return engine_state.is_debug_mode;
}

bool engine_copy_board(CBoard* out_board)
{
    if (!out_board) {
        return false;
    }

    *out_board = engine_state.board;
    return true;
}
