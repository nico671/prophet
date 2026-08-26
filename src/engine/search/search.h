#ifndef PROPHET_SEARCH_H
#define PROPHET_SEARCH_H
#include "chess/board/cboard.h"
#include "chess/movegen/move.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

// Search tuning constants
#define MAX_KILLER_MOVES 2
#define MAX_PLY 64
#define MAX_LEGAL_MOVES 256
#define TT_MOVE_SCORE 2000000
#define GOOD_CAPTURE_BASE_SCORE 1200000
#define KILLER_1_SCORE 900000
#define KILLER_2_SCORE 800000
#define BAD_CAPTURE_BASE_SCORE 100000
#define HISTORY_MAX 200000
#define MATE_SCORE 200000000
#define MATE_THRESHOLD 199999000
#define QSEARCH_DELTA_MARGIN 200
#define MOVE_OVERHEAD_DEFAULT_MS 50

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
    int multipv;
    MoveList search_moves;
} SearchLimits;

typedef struct {
    CBoard board;
    SearchLimits limits;
    bool suppress_uci_output;
} SearchInput;

typedef struct {
    uint64_t nodes;
    int64_t elapsed_ms;
    bool completed;
} SearchResult;

typedef struct {
    atomic_bool stop_requested;
    atomic_bool pondering;
    atomic_llong hard_deadline_ms;
    atomic_llong soft_limit_ms;
} SearchControl;

/**
 * @brief Reset shared controls before starting a search.
 */
void search_control_reset(SearchControl* control, bool pondering);

/**
 * @brief Request that the active search stop.
 */
void search_request_stop(SearchControl* control);

/**
 * @brief Apply normal time controls after a UCI ponderhit.
 */
void search_handle_ponder_hit(SearchControl* control, const SearchInput* input);

/**
 * @brief Execute a search synchronously.
 *
 * Handles iterative deepening, time management, UCI search reporting,
 * and dynamic soft-limit extensions (instability checks).
 */
SearchResult search_run(const SearchInput* input, SearchControl* control);

#endif // SEARCH_H
