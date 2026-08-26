#include "engine/search/search.h"

#include "chess/board/cboard.h"
#include "chess/board/zobrist.h"
#include "chess/movegen/move_make.h"
#include "chess/movegen/movegen.h"
#include "engine/eval/hceval.h"
#include "engine/search/see.h"
#include "engine/tt/tt.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    Move move;
    int score;
    int secondary_score;
} ScoredMove;

typedef struct {
    Move move;
    int score;
} RootMove;

typedef struct {
    long long soft_limit_ms;
    long long hard_limit_ms;
} TimeLimits;

/**
 * @brief Thread-local search state for a single search instance.
 *
 * All mutable search data (limits, timers, heuristics, counters) live
 * here so concurrent searches do not overwrite each other.
 */
typedef struct {
    CBoard board; // Working copy of the root board
    SearchLimits limits;
    SearchControl* control;
    Color root_side_to_move;
    long long start_time_ms;
    long long node_count;
    Move killer_moves[MAX_PLY][MAX_KILLER_MOVES];
    int history[2][64][64];
} SearchContext;

static int piece_value(PieceType piece);
static TimeLimits compute_time_limits(SearchLimits search_limits, Color side_to_move);
static bool should_stop_search(SearchContext* ctx);
static int search_root_best_move(SearchContext* ctx, CBoard* board, int depth,
                                 Move* prev_best_move);
static int search_root_moves(SearchContext* ctx, CBoard* board, int depth, Move* prev_best_move,
                             RootMove* root_moves);
static int negamax(SearchContext* ctx, CBoard* node, int depth, int alpha, int beta, Color color,
                   int ply);
static int quiescence(SearchContext* ctx, CBoard* node, int alpha, int beta, int ply);
static void score_moves(SearchContext* ctx, CBoard* board, MoveList* move_list,
                        ScoredMove* scored_moves, Move tt_move, int ply);
static void pick_next_best_move(ScoredMove* scored_moves, int start, int count);
static void clear_search_heuristics(SearchContext* ctx);
static void age_history(SearchContext* ctx);
static void adjust_history(SearchContext* ctx, Color side, Move move, int delta);
static int compare_root_moves(const void* left, const void* right);
static void print_pv_info(int depth, int multipv, int score, long long nodes, long long elapsed,
                          long long nps, const Move* pv_line, int pv_length);

static PieceType captured_piece_for_move(const CBoard* board, Move move)
{
    if (move_is_enpassant(move)) {
        return PAWN;
    }

    Square to = move_get_to_square(move);
    if (to == NO_SQUARE) {
        return NO_PIECE;
    }
    return cboard_get_piece_at_square(board, to);
}

static bool is_mate_score(int score)
{
    return score >= MATE_THRESHOLD || score <= -MATE_THRESHOLD;
}

static inline bool is_valid_square_index(Square square)
{
    int idx = (int)square;
    return idx >= 0 && idx < 64;
}

static int* history_entry(SearchContext* ctx, Color side, Square from, Square to)
{
    if (side != WHITE && side != BLACK) {
        return NULL;
    }
    if (!is_valid_square_index(from) || !is_valid_square_index(to)) {
        return NULL;
    }
    assert((int)from >= 0 && (int)from < 64);
    assert((int)to >= 0 && (int)to < 64);
    return &ctx->history[(int)side][(int)from][(int)to];
}

static int to_tt_score(int score, int ply)
{
    if (score >= MATE_THRESHOLD) {
        return score + ply;
    }
    if (score <= -MATE_THRESHOLD) {
        return score - ply;
    }
    return score;
}

static int from_tt_score(int score, int ply)
{
    if (score >= MATE_THRESHOLD) {
        return score - ply;
    }
    if (score <= -MATE_THRESHOLD) {
        return score + ply;
    }
    return score;
}

static void adjust_history(SearchContext* ctx, Color side, Move move, int delta)
{
    Square from = move_get_from_square(move);
    Square to   = move_get_to_square(move);
    int* entry  = history_entry(ctx, side, from, to);
    if (!entry) {
        return;
    }

    long long updated = (long long)(*entry) + delta;
    if (updated > HISTORY_MAX) {
        updated = HISTORY_MAX;
    } else if (updated < -HISTORY_MAX) {
        updated = -HISTORY_MAX;
    }
    *entry = (int)updated;
}

static void age_history(SearchContext* ctx)
{
    for (int side = 0; side < 2; side++) {
        for (int from = 0; from < 64; from++) {
            for (int to = 0; to < 64; to++) {
                ctx->history[side][from][to] /= 2;
            }
        }
    }
}

static void clear_search_heuristics(SearchContext* ctx)
{
    for (int ply = 0; ply < MAX_PLY; ply++) {
        ctx->killer_moves[ply][0] = MOVE_NONE;
        ctx->killer_moves[ply][1] = MOVE_NONE;
    }
    memset(ctx->history, 0, sizeof(ctx->history));
}

static int piece_value(PieceType piece)
{
    switch (piece) {
    case PAWN:
        return HC_PAWN_VALUE;
    case KNIGHT:
        return HC_KNIGHT_VALUE;
    case BISHOP:
        return HC_BISHOP_VALUE;
    case ROOK:
        return HC_ROOK_VALUE;
    case QUEEN:
        return HC_QUEEN_VALUE;
    case KING:
        return HC_KING_VALUE;
    default:
        return 0;
    }
}

static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + (long long)ts.tv_nsec / 1000000LL;
}

static bool move_allowed_by_search_moves(Move move, const MoveList* search_moves)
{
    if (search_moves->count <= 0) {
        return true;
    }

    for (int i = 0; i < search_moves->count; i++) {
        if (move == search_moves->moves[i]) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Determine whether the current search should stop.
 *
 * Checks the shared stop control, node limit, and hard deadline. The
 * time check is throttled to reduce `now_ms()` overhead.
 */
static bool should_stop_search(SearchContext* ctx)
{
    if (atomic_load(&ctx->control->stop_requested)) {
        return true;
    }

    long long nodes = ctx->node_count;
    if (ctx->limits.node_limit > 0 && nodes >= ctx->limits.node_limit) {
        atomic_store(&ctx->control->stop_requested, true);
        return true;
    }
    // only check the actual time every 2048 nodes to avoid now_ms()
    // overhead on every node
    if ((nodes & 2047) == 0) {
        long long hard_deadline = atomic_load(&ctx->control->hard_deadline_ms);
        if (!atomic_load(&ctx->control->pondering) && hard_deadline >= 0
            && now_ms() >= hard_deadline) {
            atomic_store(&ctx->control->stop_requested, true);
            return true;
        }
    }

    return false;
}

void search_control_reset(SearchControl* control, bool pondering)
{
    if (!control) {
        return;
    }

    atomic_store(&control->stop_requested, false);
    atomic_store(&control->pondering, pondering);
    atomic_store(&control->hard_deadline_ms, -1);
    atomic_store(&control->soft_limit_ms, -1);
}

void search_request_stop(SearchControl* control)
{
    if (control) {
        atomic_store(&control->stop_requested, true);
    }
}

void search_handle_ponder_hit(SearchControl* control, const SearchInput* input)
{
    if (!control || !input) {
        return;
    }

    Color side = input->board.side_to_move;
    if (side != WHITE && side != BLACK) {
        atomic_store(&control->hard_deadline_ms, -1);
        atomic_store(&control->soft_limit_ms, -1);
        atomic_store(&control->pondering, false);
        return;
    }

    TimeLimits limits          = compute_time_limits(input->limits, side);

    long long hard_deadline_ms = -1;
    long long soft_limit       = -1;

    if (limits.hard_limit_ms > 0) {
        hard_deadline_ms = now_ms() + limits.hard_limit_ms;
        soft_limit       = limits.soft_limit_ms;
    }

    atomic_store(&control->hard_deadline_ms, hard_deadline_ms);
    atomic_store(&control->soft_limit_ms, soft_limit);
    atomic_store(&control->pondering, false);
}

/**
 * @brief Compute soft/hard time limits for the current move.
 *
 * Returns {-1, -1} when running in infinite/ponder mode without time
 * controls.
 */
static TimeLimits compute_time_limits(SearchLimits search_limits, Color side_to_move)
{
    TimeLimits limits = { .soft_limit_ms = -1, .hard_limit_ms = -1 };

    // if a specific time limit was set, use that directly
    if (search_limits.time_limit_ms > 0) {
        limits.soft_limit_ms = search_limits.time_limit_ms;
        limits.hard_limit_ms = search_limits.time_limit_ms;
        return limits;
    }

    int time_remaining_ms = (side_to_move == WHITE) ? search_limits.time_for_white_ms
                                                    : search_limits.time_for_black_ms;
    // if infinite search return -1 for both limits, default value to
    // ignore time checks
    if (search_limits.infinite_search || time_remaining_ms == 0) {
        return limits;
    }

    int increment_ms = (side_to_move == WHITE) ? search_limits.increment_for_white_ms
                                               : search_limits.increment_for_black_ms;
    int moves_to_go  = search_limits.moves_until_next_time_control > 0
        ? search_limits.moves_until_next_time_control
        : 50;

    // Subtract a constant overhead (curr 50ms) to account for move
    // overhead / latency
    int safe_remaining = time_remaining_ms - MOVE_OVERHEAD_DEFAULT_MS;
    // less than 50 ms remaining so , set a very short time limit to
    // at least try to make a move instead of flagging
    if (safe_remaining <= 0) {
        limits.soft_limit_ms = 15;
        limits.hard_limit_ms = 15;
        return limits;
    }

    // soft limit, trying to spend fraction of remaining time plus
    // some of the increment (curr .5 of increment)
    long long soft_limit_ms
        = (long long)(safe_remaining / moves_to_go) + (long long)(increment_ms / 2);

    // hard limit, never spend more than abt 1/3 of remaining time on
    // single move
    long long absolute_max_time_ms = (long long)(safe_remaining / 3);

    // The hard limit is 3x the soft time, capped by the absolute max
    long long hard_time_limit_ms = soft_limit_ms * 3;
    if (hard_time_limit_ms > absolute_max_time_ms) {
        hard_time_limit_ms = absolute_max_time_ms;
    }

    limits.soft_limit_ms = soft_limit_ms;
    limits.hard_limit_ms = hard_time_limit_ms;

    // Sanity check
    if (limits.soft_limit_ms > limits.hard_limit_ms) {
        limits.soft_limit_ms = limits.hard_limit_ms;
    }

    return limits;
}

typedef struct {
    uint8_t previous_ep_square;
    uint16_t previous_halfmove_clock;
    uint16_t previous_fullmove_number;
    Color previous_side_to_move;
    uint64_t previous_zobrist_key;
} NullMoveUndo;

static NullMoveUndo make_null_move(CBoard* board)
{
    NullMoveUndo undo;
    undo.previous_ep_square       = board->ep_square;
    undo.previous_halfmove_clock  = board->half_move_clock;
    undo.previous_fullmove_number = board->full_move_number;
    undo.previous_side_to_move    = board->side_to_move;
    undo.previous_zobrist_key     = board->zobrist_key;

    zobrist_toggle_ep(&board->zobrist_key, board, board->ep_square);
    board->ep_square = NO_SQUARE;
    board->half_move_clock++;

    board->side_to_move = color_opposite(board->side_to_move);
    if (board->side_to_move == WHITE) {
        board->full_move_number++;
    }

    zobrist_toggle_side(&board->zobrist_key);

    return undo;
}

static void unmake_null_move(CBoard* board, NullMoveUndo undo)
{
    board->ep_square        = undo.previous_ep_square;
    board->half_move_clock  = undo.previous_halfmove_clock;
    board->full_move_number = undo.previous_fullmove_number;
    board->side_to_move     = undo.previous_side_to_move;
    board->zobrist_key      = undo.previous_zobrist_key;
}

/**
 * @brief Root search for the best move at a given depth.
 *
 * Applies move ordering, iterates legal root moves, and updates the
 * TT with the best score found at this depth.
 */
static int search_root_best_move(SearchContext* ctx, CBoard* board, int depth, Move* prev_best_move)
{
    MoveList move_list;
    init_move_list(&move_list);
    generate_legal_moves(board, &move_list);
    assert(move_list.count <= MAX_LEGAL_MOVES);

    if (move_list.count == 0) {
        *prev_best_move = create_move(NO_SQUARE, NO_SQUARE, 0, 0);
        return is_king_in_check(board, board->side_to_move) ? -MATE_SCORE : 0;
    }

    // check if prev best move is valid / legal in this position, if
    // so prioritize it if not fall back to probing the TT for a move
    // to prioritize
    Move tt_move              = MOVE_NONE;
    bool found_prev_best_move = move_get_from_square(*prev_best_move) != NO_SQUARE
        && move_allowed_by_search_moves(*prev_best_move, &move_list);
    if (found_prev_best_move) {
        tt_move = *prev_best_move;
    }

    if (!found_prev_best_move) {
        TTEntry* tt_entry = probe_tt(board->zobrist_key);
        if (tt_entry && tt_entry->zobrist_key == board->zobrist_key) {
            tt_move = tt_entry->best_move;
        }
    }

    ScoredMove scored_moves[MAX_LEGAL_MOVES];
    score_moves(ctx, board, &move_list, scored_moves, tt_move, 0);

    int alpha                           = -MATE_SCORE;
    int beta                            = MATE_SCORE;
    int best_score                      = -MATE_SCORE;
    Move best_move                      = create_move(NO_SQUARE, NO_SQUARE, 0, 0);
    bool has_searched_at_least_one_move = false;
    for (int i = 0; i < move_list.count; i++) {
        if (should_stop_search(ctx)) {
            break;
        }
        pick_next_best_move(scored_moves, i, move_list.count);

        Move move = scored_moves[i].move;

        // Skip moves that aren't in the search_moves list (if
        // search_moves is non-empty)
        if (!move_allowed_by_search_moves(move, &ctx->limits.search_moves)) {
            continue;
        }

        UndoInfo undo_info = make_move(board, move);
        int eval           = -negamax(ctx, board, depth - 1, -beta, -alpha, board->side_to_move, 1);
        unmake_move(board, move, undo_info);
        has_searched_at_least_one_move = true;

        if (should_stop_search(ctx)) {
            break;
        }

        if (eval > best_score || move_get_from_square(best_move) == NO_SQUARE) {
            best_score = eval;
            best_move  = move;
        }

        if (eval > alpha) {
            alpha = eval;
        }
    }

    if (!has_searched_at_least_one_move) {
        *prev_best_move = create_move(NO_SQUARE, NO_SQUARE, 0, 0);
        return -MATE_SCORE;
    }
    store_tt(board->zobrist_key, depth, to_tt_score(best_score, 0), TT_PV, best_move);
    *prev_best_move = best_move;
    return best_score;
}

static int compare_root_moves(const void* left, const void* right)
{
    const RootMove* first  = left;
    const RootMove* second = right;

    if (first->score != second->score) {
        return second->score > first->score ? 1 : -1;
    }
    return first->move > second->move ? 1 : first->move < second->move ? -1 : 0;
}

/**
 * @brief Search every allowed root move with a full window and rank the results.
 */
static int search_root_moves(SearchContext* ctx, CBoard* board, int depth, Move* prev_best_move,
                             RootMove* root_moves)
{
    MoveList move_list;
    init_move_list(&move_list);
    generate_legal_moves(board, &move_list);
    assert(move_list.count <= MAX_LEGAL_MOVES);

    if (move_list.count == 0) {
        *prev_best_move = MOVE_NONE;
        return 0;
    }

    Move tt_move              = MOVE_NONE;
    bool found_prev_best_move = move_get_from_square(*prev_best_move) != NO_SQUARE
        && move_allowed_by_search_moves(*prev_best_move, &move_list);
    if (found_prev_best_move) {
        tt_move = *prev_best_move;
    } else {
        TTEntry* tt_entry = probe_tt(board->zobrist_key);
        if (tt_entry && tt_entry->zobrist_key == board->zobrist_key) {
            tt_move = tt_entry->best_move;
        }
    }

    ScoredMove scored_moves[MAX_LEGAL_MOVES];
    score_moves(ctx, board, &move_list, scored_moves, tt_move, 0);

    int root_move_count = 0;
    for (int i = 0; i < move_list.count; i++) {
        if (should_stop_search(ctx)) {
            return 0;
        }
        pick_next_best_move(scored_moves, i, move_list.count);

        Move move = scored_moves[i].move;
        if (!move_allowed_by_search_moves(move, &ctx->limits.search_moves)) {
            continue;
        }

        UndoInfo undo_info = make_move(board, move);
        int score
            = -negamax(ctx, board, depth - 1, -MATE_SCORE, MATE_SCORE, board->side_to_move, 1);
        unmake_move(board, move, undo_info);

        if (should_stop_search(ctx)) {
            return 0;
        }

        root_moves[root_move_count++] = (RootMove) { .move = move, .score = score };
    }

    if (root_move_count == 0) {
        *prev_best_move = MOVE_NONE;
        return 0;
    }

    qsort(root_moves, (size_t)root_move_count, sizeof(*root_moves), compare_root_moves);
    store_tt(board->zobrist_key, depth, to_tt_score(root_moves[0].score, 0), TT_PV,
             root_moves[0].move);
    *prev_best_move = root_moves[0].move;
    return root_move_count;
}

static void print_pv_info(int depth, int multipv, int score, long long nodes, long long elapsed,
                          long long nps, const Move* pv_line, int pv_length)
{
    char pv_string[2048] = "";
    size_t used          = 0;
    for (int i = 0; i < pv_length; i++) {
        char move_str[6];
        move_to_uci_string(pv_line[i], move_str);

        int written = snprintf(pv_string + used, sizeof(pv_string) - used, "%s%s",
                               i == 0 ? "" : " ", move_str);
        if (written < 0 || (size_t)written >= sizeof(pv_string) - used) {
            break;
        }
        used += (size_t)written;
    }

    if (is_mate_score(score)) {
        int mate_moves = (MATE_SCORE - abs(score)) / 2;
        mate_moves     = score >= 0 ? mate_moves : -mate_moves;
        printf("info depth %d multipv %d score mate %d nodes %lld time %lld nps %lld pv %s\n",
               depth, multipv, mate_moves, nodes, elapsed, nps, pv_string);
    } else {
        printf("info depth %d multipv %d score cp %d nodes %lld time %lld nps %lld pv %s\n", depth,
               multipv, score, nodes, elapsed, nps, pv_string);
    }
}

SearchResult search_run(const SearchInput* input, SearchControl* control)
{
    SearchResult result = { 0 };
    if (!input || !control) {
        return result;
    }

    SearchContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.board             = input->board;
    ctx.limits            = input->limits;
    ctx.control           = control;
    ctx.root_side_to_move = ctx.board.side_to_move;
    ctx.start_time_ms     = now_ms();
    ctx.node_count        = 0;

    if (ctx.limits.multipv < 1) {
        ctx.limits.multipv = 1;
    } else if (ctx.limits.multipv > MAX_LEGAL_MOVES) {
        ctx.limits.multipv = MAX_LEGAL_MOVES;
    }

    // clear all killer moves / history for this search instance
    clear_search_heuristics(&ctx);

    // declare time tracking variables in the outer scope
    TimeLimits time_limits     = { .soft_limit_ms = -1, .hard_limit_ms = -1 };
    long long hard_deadline_ms = -1;
    long long soft_limit_ms    = -1;

    if (!ctx.limits.ponder) {
        time_limits = compute_time_limits(ctx.limits, ctx.root_side_to_move);

        if (time_limits.hard_limit_ms > 0) {
            hard_deadline_ms = ctx.start_time_ms + time_limits.hard_limit_ms;
            soft_limit_ms    = time_limits.soft_limit_ms;
        }
    }

    if (!ctx.limits.ponder) {
        atomic_store(&control->hard_deadline_ms, hard_deadline_ms);
        atomic_store(&control->soft_limit_ms, soft_limit_ms);
    }

    int current_depth = 1;
    int best_score    = 0;
    Move best_move    = create_move(NO_SQUARE, NO_SQUARE, 0, 0);
    // for the instability check
    Move previous_best_move = MOVE_NONE;

    int max_depth;
    if (ctx.limits.depth_limit > 0) {
        max_depth = ctx.limits.depth_limit;
    } else if (ctx.limits.infinite_search || ctx.limits.ponder) {
        max_depth = INT_MAX;
    } else {
        max_depth = 100;
    }

    // id loop
    while (current_depth <= max_depth) {
        if (should_stop_search(&ctx)) {
            break;
        }

        RootMove root_moves[MAX_LEGAL_MOVES];
        int root_move_count = 0;
        if (ctx.limits.multipv > 1) {
            Move depth_best_move = best_move;
            root_move_count
                = search_root_moves(&ctx, &ctx.board, current_depth, &depth_best_move, root_moves);
        } else {
            Move depth_best_move = best_move;
            int score = search_root_best_move(&ctx, &ctx.board, current_depth, &depth_best_move);
            if (move_get_from_square(depth_best_move) != NO_SQUARE) {
                root_moves[0]   = (RootMove) { .move = depth_best_move, .score = score };
                root_move_count = 1;
            }
        }

        if (should_stop_search(&ctx)) {
            break;
        }
        if (root_move_count > 0) {
            best_move  = root_moves[0].move;
            best_score = root_moves[0].score;
        }

        long long elapsed = now_ms() - ctx.start_time_ms;
        long long nodes   = ctx.node_count;
        long long nps     = elapsed > 0 ? (nodes * 1000LL) / elapsed : nodes;

        int report_count
            = root_move_count < ctx.limits.multipv ? root_move_count : ctx.limits.multipv;
        if (!input->suppress_uci_output) {
            for (int rank = 0; rank < report_count; rank++) {
                Move pv_line[MAX_LEGAL_MOVES];
                int pv_limit    = current_depth < MAX_LEGAL_MOVES ? current_depth : MAX_LEGAL_MOVES;
                pv_line[0]      = root_moves[rank].move;

                CBoard pv_board = ctx.board;
                make_move(&pv_board, pv_line[0]);
                int pv_length = 1 + extract_pv_line(&pv_board, pv_line + 1, pv_limit - 1);
                print_pv_info(current_depth, rank + 1, root_moves[rank].score, nodes, elapsed, nps,
                              pv_line, pv_length);
            }
        }
        if (!input->suppress_uci_output) {
            fflush(stdout);
        }

        age_history(&ctx);
        current_depth++;

        // time management checks after each depth iteration
        long long current_soft_limit = atomic_load(&control->soft_limit_ms);

        if (ctx.limits.time_limit_ms == 0 && current_soft_limit > 0) {
            // dynamically increase the soft limit if we detect
            // instability in the root move (best move changes from
            // previous iteration) after depth 2
            if (current_depth > 2 && best_move != previous_best_move) {
                current_soft_limit += current_soft_limit / 2;
                atomic_store(&control->soft_limit_ms,
                             current_soft_limit); // Save the extended time
            }

            // Soft Limit Break
            if (elapsed >= current_soft_limit) {
                break;
            }

            // Early Abort Optimization
            if (time_limits.hard_limit_ms > 0 && elapsed > (time_limits.hard_limit_ms * 0.6)) {
                break;
            }
        }

        //  update previous_best_move for the next depth's instability
        //  check
        previous_best_move = best_move;

        if (!atomic_load(&control->pondering) && ctx.limits.search_for_mate_in_n_moves > 0
            && is_mate_score(best_score)) {
            int mate_moves = (MATE_SCORE - abs(best_score)) / 2;
            if (mate_moves <= ctx.limits.search_for_mate_in_n_moves) {
                break;
            }
        }
    }

    if (!input->suppress_uci_output) {
        char best_move_string[6];
        move_to_uci_string(best_move, best_move_string);
        printf("bestmove %s\n", best_move_string);
        fflush(stdout);
    }

    result.nodes      = (uint64_t)ctx.node_count;
    result.elapsed_ms = (int64_t)(now_ms() - ctx.start_time_ms);
    result.completed  = !atomic_load(&control->stop_requested);
    return result;
}

static void score_moves(SearchContext* ctx, CBoard* board, MoveList* move_list,
                        ScoredMove* scored_moves, Move tt_move, int ply)
{
    Color side = board->side_to_move;

    for (int i = 0; i < move_list->count; i++) {
        Move curr_move       = move_list->moves[i];
        int score            = 0;
        int secondary_score  = 0;

        scored_moves[i].move = curr_move;
        if (tt_move != MOVE_NONE && curr_move == tt_move) {
            score = TT_MOVE_SCORE;
        } else if (move_is_capture(board, curr_move) || move_is_promotion(curr_move)) {
            PieceType captured_piecetype = captured_piece_for_move(board, curr_move);
            int see                      = see_capture(board, curr_move);
            if (see >= 0) {
                score = GOOD_CAPTURE_BASE_SCORE + see;
            } else {
                score = BAD_CAPTURE_BASE_SCORE + see;
            }
            secondary_score = piece_value(captured_piecetype);
        } else if (ply < MAX_PLY && curr_move == ctx->killer_moves[ply][0]) {
            score = KILLER_1_SCORE;
        } else if (ply < MAX_PLY && curr_move == ctx->killer_moves[ply][1]) {
            score = KILLER_2_SCORE;
        } else {
            Square from = move_get_from_square(curr_move);
            Square to   = move_get_to_square(curr_move);
            int* entry  = history_entry(ctx, side, from, to);
            if (entry) {
                score = *entry;
            }
        }
        scored_moves[i].score           = score;
        scored_moves[i].secondary_score = secondary_score;
    }
}

static void pick_next_best_move(ScoredMove* scored_moves, int start, int count)
{
    for (int i = start; i < count; i++) {
        bool better_score = scored_moves[i].score > scored_moves[start].score;
        bool equal_score  = scored_moves[i].score == scored_moves[start].score;
        bool better_secondary
            = scored_moves[i].secondary_score > scored_moves[start].secondary_score;
        if (better_score || (equal_score && better_secondary)) {
            // Swap
            ScoredMove temp     = scored_moves[start];
            scored_moves[start] = scored_moves[i];
            scored_moves[i]     = temp;
        }
    }
}

/**
 * @brief Quiescence search to stabilize tactical positions.
 *
 * Searches captures/promotions (and evasions if in check) to mitigate
 * the horizon effect.
 */
static int quiescence(SearchContext* ctx, CBoard* node, int alpha, int beta, int ply)
{
    ctx->node_count++;

    if (should_stop_search(ctx)) {
        return hc_evaluate_cboard(node);
    }

    if (ply >= MAX_PLY - 1) {
        return hc_evaluate_cboard(node);
    }

    bool king_in_check = is_king_in_check(node, node->side_to_move);
    int stand_pat      = hc_evaluate_cboard(node);
    if (!king_in_check) {
        if (stand_pat >= beta) {
            return stand_pat;
        }
        if (stand_pat > alpha) {
            alpha = stand_pat;
        }
    }

    MoveList move_list;
    init_move_list(&move_list);
    if (king_in_check) {
        generate_legal_moves(node, &move_list);
    } else {
        generate_capture_moves(node, &move_list);
    }

    if (move_list.count == 0) {
        if (king_in_check) {
            return -MATE_SCORE + ply;
        }
        return stand_pat;
    }

    TTEntry* tt_entry = probe_tt(node->zobrist_key);
    Move tt_best_move = MOVE_NONE;
    if (tt_entry && tt_entry->zobrist_key == node->zobrist_key) {
        tt_best_move = tt_entry->best_move;
    }

    assert(move_list.count <= MAX_LEGAL_MOVES);
    ScoredMove scored_moves[MAX_LEGAL_MOVES];
    score_moves(ctx, node, &move_list, scored_moves, tt_best_move, ply);

    for (int i = 0; i < move_list.count; i++) {
        if (should_stop_search(ctx)) {
            break;
        }

        pick_next_best_move(scored_moves, i, move_list.count);
        Move move           = scored_moves[i].move;

        bool capture        = move_is_capture(node, move);
        int optimistic_gain = 0;
        if (capture) {
            optimistic_gain = piece_value(captured_piece_for_move(node, move));
            if (move_is_promotion(move)) {
                optimistic_gain
                    += piece_value(move_get_promotion_piecetype(move)) - piece_value(PAWN);
            }
        }

        bool prune_capture = !king_in_check && capture && !move_is_promotion(move)
            && see_capture(node, move) < -HC_PAWN_VALUE;
        UndoInfo undo_info = make_move(node, move);
        bool gives_check   = is_king_in_check(node, node->side_to_move);
        if (prune_capture && !gives_check) {
            unmake_move(node, move, undo_info);
            continue;
        }

        bool prune_delta = capture && !king_in_check && !move_is_promotion(move)
            && !is_mate_score(alpha) && !is_mate_score(beta) && !gives_check
            && !should_stop_search(ctx)
            && stand_pat + optimistic_gain + QSEARCH_DELTA_MARGIN <= alpha;
        if (prune_delta) {
            unmake_move(node, move, undo_info);
            continue;
        }

        int eval = -quiescence(ctx, node, -beta, -alpha, ply + 1);
        unmake_move(node, move, undo_info);

        if (eval >= beta) {
            return eval;
        }
        if (eval > alpha) {
            alpha = eval;
        }
    }

    return alpha;
}

/**
 * @brief Alpha-beta negamax with TT, null-move pruning, and PVS/LMR.
 */
static int negamax(SearchContext* ctx, CBoard* node, int depth, int alpha, int beta, Color color,
                   int ply)
{
    ctx->node_count++;

    if (should_stop_search(ctx)) {
        return hc_evaluate_cboard(node);
    }

    if (ply >= MAX_PLY - 1) {
        return quiescence(ctx, node, alpha, beta, ply);
    }

    Color side         = color;
    int original_alpha = alpha;
    bool king_in_check = is_king_in_check(node, side);

    TTEntry* tt_entry  = probe_tt(node->zobrist_key);
    Move tt_best_move  = MOVE_NONE;
    if (tt_entry && tt_entry->zobrist_key == node->zobrist_key) {
        tt_best_move = tt_entry->best_move;
        int tt_score = from_tt_score(tt_entry->score, ply);
        if (tt_entry->depth >= depth) {
            if (tt_entry->bound == TT_PV) {
                return tt_score;
            } else if (tt_entry->bound == TT_CUT && tt_score >= beta) {
                return tt_score;
            } else if (tt_entry->bound == TT_ALL && tt_score <= alpha) {
                return tt_score;
            }
            if (alpha >= beta) {
                return tt_score;
            }
        }
    }

    if (depth == 0) {
        return quiescence(ctx, node, alpha, beta, ply);
    }

    // Null-move pruning (skip in check or near-mate windows)
    if (!king_in_check && depth >= 3 && !is_mate_score(alpha) && !is_mate_score(beta)) {
        int reduction               = 2 + (depth >= 6 ? 1 : 0);
        NullMoveUndo null_undo_info = make_null_move(node);
        Color next_side             = color_opposite(side);
        int null_score
            = -negamax(ctx, node, depth - 1 - reduction, -beta, -beta + 1, next_side, ply + 1);
        unmake_null_move(node, null_undo_info);

        if (null_score >= beta) {
            return null_score;
        }
    }

    MoveList move_list;
    init_move_list(&move_list);
    gen_all_pseudolegal_moves(node, &move_list);
    assert(move_list.count <= MAX_LEGAL_MOVES);
    ScoredMove scored_moves[MAX_LEGAL_MOVES];
    score_moves(ctx, node, &move_list, scored_moves, tt_best_move, ply);

    Move best_move_at_node       = create_move(NO_SQUARE, NO_SQUARE, 0, 0);
    int max_eval                 = -MATE_SCORE;
    int num_legal_moves_searched = 0;

    for (int i = 0; i < move_list.count; i++) {
        if (should_stop_search(ctx)) {
            break;
        }
        pick_next_best_move(scored_moves, i, move_list.count);
        Move move  = scored_moves[i].move;

        bool quiet = move_is_quiet(node, move);

        // Make the move
        UndoInfo undo_info = make_move(node, move);

        if (is_king_in_check(node, side)) {
            unmake_move(node, move, undo_info);
            continue;
        }

        num_legal_moves_searched++;
        Color next_side = color_opposite(side);
        int eval;

        if (num_legal_moves_searched == 1) {
            eval = -negamax(ctx, node, depth - 1, -beta, -alpha, next_side, ply + 1);
        } else {
            int reduced_depth = depth - 1;
            bool can_reduce_depth
                = depth >= 3 && num_legal_moves_searched >= 4 && !king_in_check && quiet;
            if (can_reduce_depth) {
                reduced_depth = depth - 2;
            }

            // PVS scout search with null window (\alpha , \alpha + 1)
            eval = -negamax(ctx, node, reduced_depth, -alpha - 1, -alpha, next_side, ply + 1);

            // Re-search if scout failed high (score > alpha), search
            // with full window
            if (eval > alpha) {
                eval = -negamax(ctx, node, depth - 1, -beta, -alpha, next_side, ply + 1);
            }
        }

        // Unmake the move
        unmake_move(node, move, undo_info);

        int prev_alpha = alpha;

        if (eval > max_eval) {
            max_eval          = eval;
            best_move_at_node = move;
        }
        if (max_eval > alpha) {
            alpha = max_eval;
        } else if (quiet && eval <= prev_alpha) {
            adjust_history(ctx, side, move, -(depth * depth));
        }

        if (alpha >= beta) {
            // Store killer move
            if (ply < MAX_PLY && quiet) {
                if (ctx->killer_moves[ply][0] != move) {
                    ctx->killer_moves[ply][1] = ctx->killer_moves[ply][0];
                    ctx->killer_moves[ply][0] = move;
                }
                adjust_history(ctx, side, move, depth * depth);
            }
            break; // beta cutoff
        }
    }

    if (num_legal_moves_searched == 0) {
        if (king_in_check) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    TTBound bound = TT_PV;
    if (max_eval <= original_alpha) {
        bound = TT_ALL;
    } else if (max_eval >= beta) {
        bound = TT_CUT;
    }
    store_tt(node->zobrist_key, depth, to_tt_score(max_eval, ply), bound, best_move_at_node);
    return max_eval;
}
