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
#define ROOT_ASPIRATION_START_DEPTH 4
#define ROOT_ASPIRATION_INITIAL_WINDOW 50
#define MOVE_OVERHEAD_DEFAULT_MS 50
#define MAX_POSITION_HISTORY 151

/**
 * Positions in the current game since the last pawn move or capture.
 * The final key is the current position.
 */
typedef struct {
    uint64_t keys[MAX_POSITION_HISTORY];
    uint16_t count;
} PositionHistory;

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
    PositionHistory position_history;
    SearchLimits limits;
    bool suppress_uci_output;
} SearchInput;

typedef struct {
    Move move;
    int score;
} SearchRootLine;

typedef struct {
    uint64_t nodes;
    int64_t elapsed_ms;
    /** Best move from the last fully completed iteration, or MOVE_NONE. */
    Move best_move;
    /** Root score from the original side-to-move perspective. */
    int score;
    /** Deepest fully completed iterative-deepening iteration, or zero. */
    int completed_depth;
    /** True when the search was not interrupted; requested depth may not be reached. */
    bool completed;
    /** Ranked root lines from the last fully completed iteration. */
    SearchRootLine root_lines[MAX_LEGAL_MOVES];
    /** Number of valid entries in root_lines. */
    int root_line_count;
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
