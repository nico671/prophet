#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "board/cboard.h"
#include "board/zobrist.h"
#include "engine/engine.h"
#include "eval/hceval.h"
#include "movegen/move_make.h"
#include "movegen/movegen.h"
#include "search/search.h"
#include "search/tt.h"

// time limits struct for soft / hard time control limits
typedef struct {
    long long soft_limit_ms;
    long long hard_limit_ms;
} TimeLimits;

Move killer_moves_list[MAX_PLY][MAX_KILLER_MOVES]; // [depth][idx] where 0 is newest
                                                   // killer, 1 is previous killer
int history_heuristic[2][64][64]; // [color][from][to]

static int piece_value(PieceType piece);
static long long compute_time_budget_ms(const CBoard* board, SearchLimits search_limits);

static const int TT_MOVE_SCORE = 2000000;
static const int GOOD_CAPTURE_BASE_SCORE = 1200000;
static const int KILLER_1_SCORE = 900000;
static const int KILLER_2_SCORE = 800000;
static const int BAD_CAPTURE_BASE_SCORE = 100000;
static const int HISTORY_MAX = 200000;
static const int MATE_SCORE = 200000000;
static const int MATE_THRESHOLD = 199999000;
static const int MOVE_OVERHEAD_DEFAULT_MS = 50;
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

static bool is_promotion_capture(const CBoard* board, Move move)
{
    if (!move_is_promotion(move)) {
        return false;
    }

    Square to = move_get_to_square(move);
    if (to == NO_SQUARE) {
        return false;
    }
    return bitboard_is_bit_set(board->occupancy_bbs[1 - board->side_to_move], to);
}

static bool is_capture_like(CBoard* board, Move move)
{
    return move_is_capture(board, move) || move_is_enpassant(move) || is_promotion_capture(board, move);
}

static bool is_quiet_move(CBoard* board, Move move)
{
    return !move_is_capture(board, move) && !move_is_enpassant(move) && !move_is_promotion(move);
}

static bool is_mate_score(int score)
{
    return score >= MATE_THRESHOLD || score <= -MATE_THRESHOLD;
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

static void update_history(Color side, Move move, int depth)
{
    if (side != WHITE && side != BLACK) {
        return;
    }

    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);
    if (from == NO_SQUARE || to == NO_SQUARE) {
        return;
    }

    int bonus = depth * depth;
    int* entry = &history_heuristic[(int)side][(int)from][(int)to];
    long long updated = (long long)(*entry) + bonus;
    if (updated > HISTORY_MAX) {
        updated = HISTORY_MAX;
    } else if (updated < -HISTORY_MAX) {
        updated = -HISTORY_MAX;
    }
    *entry = (int)updated;
}

static void penalize_history(Color side, Move move, int depth)
{
    if (side != WHITE && side != BLACK) {
        return;
    }

    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);
    if (from == NO_SQUARE || to == NO_SQUARE) {
        return;
    }

    int malus = depth * depth;
    int* entry = &history_heuristic[(int)side][(int)from][(int)to];
    long long updated = (long long)(*entry) - malus;
    if (updated > HISTORY_MAX) {
        updated = HISTORY_MAX;
    } else if (updated < -HISTORY_MAX) {
        updated = -HISTORY_MAX;
    }
    *entry = (int)updated;
}

static void age_history(void)
{
    for (int side = 0; side < 2; side++) {
        for (int from = 0; from < 64; from++) {
            for (int to = 0; to < 64; to++) {
                history_heuristic[side][from][to] /= 2;
            }
        }
    }
}

void clear_search_heuristics(void)
{
    for (int ply = 0; ply < MAX_PLY; ply++) {
        killer_moves_list[ply][0] = MOVE_NONE;
        killer_moves_list[ply][1] = MOVE_NONE;
    }
    memset(history_heuristic, 0, sizeof(history_heuristic));
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

static SearchLimits active_search_limits;
static atomic_llong search_node_count = 0;
static long long search_start_ms = 0;
static atomic_llong search_deadline_ms = -1;
static atomic_int active_search_side_to_move = WHITE;

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

static bool should_stop_search(void)
{
    if (atomic_load(&search_stop_flag)) {
        return true;
    }

    long long nodes = atomic_load(&search_node_count);
    if (active_search_limits.node_limit > 0 && nodes >= active_search_limits.node_limit) {
        atomic_store(&search_stop_flag, true);
        return true;
    }
    // only check the actual time every 2048 nodes to avoid now_ms() overhead on every node
    if ((nodes & 2047) == 0) {
        long long deadline = atomic_load(&search_deadline_ms);
        if (!atomic_load(&search_is_pondering) && deadline >= 0 && now_ms() >= deadline) {
            atomic_store(&search_stop_flag, true);
            return true;
        }
    }

    return false;
}

void on_ponder_hit(void)
{
    atomic_store(&search_is_pondering, false);

    Color side = (Color)atomic_load(&active_search_side_to_move);
    if (side != WHITE && side != BLACK) {
        atomic_store(&search_deadline_ms, -1);
        return;
    }

    CBoard budget_cboard = { 0 };
    budget_cboard.side_to_move = side;

    long long budget_ms = compute_time_budget_ms(&budget_cboard, active_search_limits);
    long long deadline_ms = (budget_ms > 0) ? (now_ms() + budget_ms) : -1;
    atomic_store(&search_deadline_ms, deadline_ms);
}

static TimeLimits compute_time_limits(const CBoard* board, SearchLimits search_limits)
{
    TimeLimits limits = { -1, -1 };

    // if a specific time limit was set, use that directly
    if (search_limits.time_limit_ms > 0) {
        limits.soft_limit_ms = search_limits.time_limit_ms;
        limits.hard_limit_ms = search_limits.time_limit_ms;
        return limits;
    }

    // if infinite search return -1 for both limits, default value to ignore time checks
    if (search_limits.infinite_search) {
        return limits;
    }

    int time_remaining_ms = (board->side_to_move == WHITE) ? search_limits.time_for_white_ms
                                                           : search_limits.time_for_black_ms;
    int increment_ms = (board->side_to_move == WHITE) ? search_limits.increment_for_white_ms
                                                      : search_limits.increment_for_black_ms;
    int moves_to_go = search_limits.moves_until_next_time_control > 0 ? search_limits.moves_until_next_time_control : 50;

    // Subtract a constant overhead (curr 50ms) to account for move overhead / latency
    int safe_remaining = time_remaining_ms - MOVE_OVERHEAD_DEFAULT_MS;
    // less than 50 ms remaining so , set a very short time limit to at least try to make a move instead of flagging
    if (safe_remaining <= 0) {
        limits.soft_limit_ms = 15;
        limits.hard_limit_ms = 15;
        return limits;
    }

    // soft limit, trying to spend fraction of remaining time plus some of the increment (curr .5 of increment)
    long long soft_limit_ms = (long long)(safe_remaining / moves_to_go) + (long long)(increment_ms / 2);

    // hard limit, never spend more than abt 1/3 of remaining time on single move
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
    uint16_t previousFullmoveNumber;
    Color previousSideToMove;
    uint64_t previous_zobrist_key;
} NullMoveUndo;

static NullMoveUndo make_null_move(CBoard* board)
{
    NullMoveUndo undo;
    undo.previous_ep_square = board->ep_square;
    undo.previous_halfmove_clock = board->half_move_clock;
    undo.previousFullmoveNumber = board->full_move_number;
    undo.previousSideToMove = board->side_to_move;
    undo.previous_zobrist_key = board->zobrist_key;

    zobrist_toggle_ep(&board->zobrist_key, board, board->ep_square);
    board->ep_square = NO_SQUARE;
    board->half_move_clock++;

    board->side_to_move = 1 - board->side_to_move;
    if (board->side_to_move == WHITE) {
        board->full_move_number++;
    }

    zobrist_toggle_side(&board->zobrist_key);

    return undo;
}

static void unmake_null_move(CBoard* board, NullMoveUndo undo)
{
    board->ep_square = undo.previous_ep_square;
    board->half_move_clock = undo.previous_halfmove_clock;
    board->full_move_number = undo.previousFullmoveNumber;
    board->side_to_move = undo.previousSideToMove;
    board->zobrist_key = undo.previous_zobrist_key;
}

static void move_to_uci_string(Move move, char* out)
{
    if (move_get_from_square(move) == NO_SQUARE || move_get_to_square(move) == NO_SQUARE) {
        strcpy(out, "0000");
        return;
    }

    out[0] = (char)('a' + (move_get_from_square(move) % 8));
    out[1] = (char)('1' + (move_get_from_square(move) / 8));
    out[2] = (char)('a' + (move_get_to_square(move) % 8));
    out[3] = (char)('1' + (move_get_to_square(move) / 8));

    if (move_is_promotion(move)) {
        PieceType promo = move_get_promotion_piecetype(move);
        char promo_char = 'q';
        if (promo == KNIGHT)
            promo_char = 'n';
        else if (promo == BISHOP)
            promo_char = 'b';
        else if (promo == ROOK)
            promo_char = 'r';
        else
            promo_char = 'q';

        out[4] = promo_char;
        out[5] = '\0';
        return;
    }

    out[4] = '\0';
}

static int search_root_best_move(CBoard* board, int depth, Move* prev_best_move)
{
    MoveList move_list;
    init_move_list(&move_list);
    generate_legal_moves(board, &move_list);

    if (move_list.count == 0) {
        *prev_best_move = create_move(NO_SQUARE, NO_SQUARE, 0, 0);
        return is_king_in_check(board, board->side_to_move) ? -MATE_SCORE : 0;
    }

    // check if prev best move is valid / legal in this position, if so prioritize
    // it if not fall back to probing the TT for a move to prioritize
    Move tt_move = MOVE_NONE;
    bool found_prev_best_move = false;
    if (move_get_from_square(*prev_best_move) != NO_SQUARE) {
        for (int i = 0; i < move_list.count; i++) {
            if (move_list.moves[i] == *prev_best_move) {
                found_prev_best_move = true;
                break;
            }
        }
        if (found_prev_best_move) {
            tt_move = *prev_best_move;
        }
    }

    if (!found_prev_best_move) {
        TTEntry* tt_entry = probe_tt(board->zobrist_key);
        if (tt_entry && tt_entry->zobrist_key == board->zobrist_key) {
            tt_move = tt_entry->best_move;
        }
    }

    ScoredMove scored_moves[256];
    score_moves(board, &move_list, scored_moves, tt_move, 0);

    int alpha = -200000000;
    int beta = 200000000;
    int best_score = -MATE_SCORE;
    Move best_move = create_move(NO_SQUARE, NO_SQUARE, 0, 0);
    bool has_searched_at_least_one_move = false;
    for (int i = 0; i < move_list.count; i++) {
        if (should_stop_search()) {
            break;
        }
        pick_next_best_move(scored_moves, i, move_list.count);

        Move move = scored_moves[i].move;

        char curr_move_uci_string[6];
        move_to_uci_string(move, curr_move_uci_string);
        // printf("info depth %d currmove %s currmovenumber %d\n", depth,
        // curr_move_uci_string, i + 1);

        // Skip moves that aren't in the search_moves list (if search_moves is
        // non-empty)
        if (!move_allowed_by_search_moves(move, &active_search_limits.search_moves)) {
            continue;
        }

        UndoInfo undo_info = make_move(board, move);
        int eval = -negamax(board, depth - 1, -beta, -alpha, board->side_to_move, 1);
        unmake_move(board, move, undo_info);
        has_searched_at_least_one_move = true;

        if (should_stop_search()) {
            break;
        }

        if (eval > best_score || move_get_from_square(best_move) == NO_SQUARE) {
            best_score = eval;
            best_move = move;
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

// The entry point for the search thread
void* search_worker(void* arg)
{
    // Cast and extract the data
    SearchThreadData* data = (SearchThreadData*)arg;
    CBoard search_board = data->board;
    SearchLimits search_limits = data->search_limits;
    // clear all killer moves
    clear_search_heuristics();
    active_search_limits = search_limits;
    atomic_store(&active_search_side_to_move, (int)search_board.side_to_move);
    atomic_store(&search_node_count, 0);
    search_start_ms = now_ms();
    long long deadline_ms = -1;
    if (!search_limits.ponder) {
        long long budget_ms = compute_time_budget_ms(&search_board, search_limits);
        deadline_ms = (budget_ms > 0) ? (search_start_ms + budget_ms) : -1;
    }
    atomic_store(&search_deadline_ms, deadline_ms);

    // We copied the data to local stack variables, so free the allocated payload
    free(data);

    int current_depth = 1;
    int best_score = 0;
    Move best_move = create_move(NO_SQUARE, NO_SQUARE, 0, 0);
    Move ponder_move = MOVE_NONE;

    int max_depth;
    if (search_limits.depth_limit > 0) {
        max_depth = search_limits.depth_limit;
    } else if (search_limits.infinite_search || search_limits.ponder) {
        max_depth = INT_MAX;
    } else {
        max_depth = 100;
    }

    // The Iterative Deepening Loop
    while (current_depth <= max_depth) {
        if (should_stop_search()) {
            break;
        }

        Move depth_best_move = best_move;
        int score = search_root_best_move(&search_board, current_depth, &depth_best_move);
        if (!should_stop_search() && move_get_from_square(depth_best_move) != NO_SQUARE) {
            best_move = depth_best_move;
            best_score = score;
        }

        long long elapsed = now_ms() - search_start_ms;
        long long nodes = atomic_load(&search_node_count);
        long long nps = elapsed > 0 ? (nodes * 1000LL) / elapsed : nodes;
        Move pv_line[256];
        int pv_length = extract_pv_line(&search_board, pv_line, current_depth);
        if (pv_length >= 2 && pv_line[0] == best_move) {
            ponder_move = pv_line[1];
        }
        char pv_string[2048] = "";
        for (int i = 0; i < pv_length; i++) {
            char move_str[6];
            move_to_uci_string(pv_line[i], move_str);
            strcat(pv_string, move_str);
            strcat(pv_string, " ");
        }
        if (abs(best_score) > 100000000) {
            int mate_moves = (MATE_SCORE - abs(best_score)) / 2;
            int mate_score = best_score >= 0 ? mate_moves : -mate_moves;
            printf(
                "info depth %d score mate %d nodes %lld time %lld nps %lld pv %s\n",
                current_depth, mate_score, nodes, elapsed, nps, pv_string);
        } else {
            printf("info depth %d score cp %d nodes %lld time %lld nps %lld pv %s\n",
                current_depth, best_score, nodes, elapsed, nps, pv_string);
        }

        age_history();

        current_depth++;

        if (search_limits.time_limit_ms > 0 && should_stop_search()) {
            break;
        }

        if (!atomic_load(&search_is_pondering) && search_limits.search_for_mate_in_n_moves > 0 && abs(best_score) > 100000000) {
            int mate_moves = (MATE_SCORE - abs(best_score)) / 2;
            if (mate_moves <= search_limits.search_for_mate_in_n_moves) {
                break;
            }
        }
    }

    char best_move_uci_string[6];
    move_to_uci_string(best_move, best_move_uci_string);
    if (move_get_from_square(ponder_move) != NO_SQUARE && move_get_to_square(ponder_move) != NO_SQUARE) {
        char ponder_move_uci_string[6];
        move_to_uci_string(ponder_move, ponder_move_uci_string);
        printf("bestmove %s ponder %s\n", best_move_uci_string, ponder_move_uci_string);
    } else {
        printf("bestmove %s\n", best_move_uci_string);
    }
    fflush(stdout);

    return NULL;
}

void score_moves(CBoard* board, MoveList* move_list, ScoredMove* scored_moves,
    Move tt_move, int ply)
{
    Color side = board->side_to_move;

    for (int i = 0; i < move_list->count; i++) {
        Move curr_move = move_list->moves[i];
        int score = 0;

        scored_moves[i].move = curr_move;
        if (tt_move != MOVE_NONE && curr_move == tt_move) {
            score = TT_MOVE_SCORE;
        } else if (is_capture_like(board, curr_move) || move_is_promotion(curr_move)) {
            PieceType attacker_piecetype = cboard_get_piece_at_square(board, move_get_from_square(curr_move));
            PieceType captured_piecetype = captured_piece_for_move(board, curr_move);
            int mvv_lva = 0;
            if (captured_piecetype != NO_PIECE && attacker_piecetype != NO_PIECE) {
                mvv_lva = piece_value(captured_piecetype) - piece_value(attacker_piecetype);
            }

            int promo_bonus = 0;
            if (move_is_promotion(curr_move)) {
                promo_bonus = piece_value(move_get_promotion_piecetype(curr_move));
            }

            if (mvv_lva >= 0 || move_is_promotion(curr_move)) {
                score = GOOD_CAPTURE_BASE_SCORE + mvv_lva + promo_bonus;
            } else {
                score = BAD_CAPTURE_BASE_SCORE + mvv_lva + promo_bonus;
            }
        } else if (ply < MAX_PLY) {
            if (curr_move == killer_moves_list[ply][0]) {
                score = KILLER_1_SCORE;
            } else if (curr_move == killer_moves_list[ply][1]) {
                score = KILLER_2_SCORE;
            } else {
                Square from = move_get_from_square(curr_move);
                Square to = move_get_to_square(curr_move);
                if (from != NO_SQUARE && to != NO_SQUARE) {
                    score = history_heuristic[(int)side][(int)from][(int)to];
                }
            }
        } else {
            Square from = move_get_from_square(curr_move);
            Square to = move_get_to_square(curr_move);
            if (from != NO_SQUARE && to != NO_SQUARE) {
                score = history_heuristic[(int)side][(int)from][(int)to];
            }
        }
        scored_moves[i].score = score;
    }
}

void pick_next_best_move(ScoredMove* scored_moves, int start, int count)
{
    for (int i = start; i < count; i++) {
        if (scored_moves[i].score > scored_moves[start].score) {
            // Swap
            ScoredMove temp = scored_moves[start];
            scored_moves[start] = scored_moves[i];
            scored_moves[i] = temp;
        }
    }
}

int quiescence(CBoard* node, int alpha, int beta, int ply)
{
    atomic_fetch_add(&search_node_count, 1);

    if (should_stop_search()) {
        return evaluate_cboard(node);
    }

    if (ply >= MAX_PLY - 1) {
        return evaluate_cboard(node);
    }

    bool king_in_check = is_king_in_check(node, node->side_to_move);
    if (!king_in_check) {
        int stand_pat = evaluate_cboard(node);
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
        return evaluate_cboard(node);
    }

    TTEntry* tt_entry = probe_tt(node->zobrist_key);
    Move tt_best_move = MOVE_NONE;
    if (tt_entry && tt_entry->zobrist_key == node->zobrist_key) {
        tt_best_move = tt_entry->best_move;
    }

    ScoredMove scored_moves[256];
    score_moves(node, &move_list, scored_moves, tt_best_move, ply);

    for (int i = 0; i < move_list.count; i++) {
        if (should_stop_search()) {
            break;
        }

        pick_next_best_move(scored_moves, i, move_list.count);
        Move move = scored_moves[i].move;

        if (!king_in_check && !is_capture_like(node, move) && !move_is_promotion(move)) {
            continue;
        }

        UndoInfo undo_info = make_move(node, move);
        int eval = -quiescence(node, -beta, -alpha, ply + 1);
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

int negamax(CBoard* node, int depth, int alpha, int beta, Color color,
    int ply)
{
    atomic_fetch_add(&search_node_count, 1);

    if (should_stop_search()) {
        return evaluate_cboard(node);
    }

    if (ply >= MAX_PLY - 1) {
        return quiescence(node, alpha, beta, ply);
    }

    Color side = color;
    int original_alpha = alpha;
    bool king_in_check = is_king_in_check(node, side);

    TTEntry* tt_entry = probe_tt(node->zobrist_key);
    Move tt_best_move = MOVE_NONE;
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
        return quiescence(node, alpha, beta, ply);
    }

    // Null-move pruning (skip in check or near-mate windows)
    if (!king_in_check && depth >= 3 && !is_mate_score(alpha) && !is_mate_score(beta)) {
        int reduction = 2 + (depth >= 6 ? 1 : 0);
        NullMoveUndo null_undo_info = make_null_move(node);
        Color next_side = 1 - side;
        int null_score = -negamax(node, depth - 1 - reduction, -beta, -beta + 1,
            next_side, ply + 1);
        unmake_null_move(node, null_undo_info);

        if (null_score >= beta) {
            return null_score;
        }
    }

    MoveList move_list;
    init_move_list(&move_list);
    gen_all_pseudolegal_moves(node, &move_list);
    ScoredMove scored_moves[256];
    score_moves(node, &move_list, scored_moves, tt_best_move, ply);

    Move best_move_at_node = create_move(NO_SQUARE, NO_SQUARE, 0, 0);
    int max_eval = -MATE_SCORE;
    int num_legal_moves_searched = 0;

    for (int i = 0; i < move_list.count; i++) {
        if (should_stop_search()) {
            break;
        }
        pick_next_best_move(scored_moves, i, move_list.count);
        Move move = scored_moves[i].move;

        bool quiet = is_quiet_move(node, move);

        // Make the move
        UndoInfo undo_info = make_move(node, move);

        if (is_king_in_check(node, side)) {
            unmake_move(node, move, undo_info);
            continue;
        }

        num_legal_moves_searched++;
        Color next_side = 1 - side;
        int eval;

        if (num_legal_moves_searched == 1) {
            eval = -negamax(node, depth - 1, -beta, -alpha, next_side, ply + 1);
        } else {
            int reduced_depth = depth - 1;
            bool can_reduce_depth = depth >= 3 && num_legal_moves_searched >= 4 && !king_in_check && quiet;
            if (can_reduce_depth) {
                reduced_depth = depth - 2;
            }

            // PVS scout search
            eval = -negamax(node, reduced_depth, -alpha - 1, -alpha, next_side, ply + 1);

            // Re-search if scout failed high
            if (eval > alpha) {
                eval = -negamax(node, depth - 1, -beta, -alpha, next_side, ply + 1);
            }
        }

        // Unmake the move
        unmake_move(node, move, undo_info);

        int prev_alpha = alpha;

        if (eval > max_eval) {
            max_eval = eval;
            best_move_at_node = move;
        }
        if (max_eval > alpha) {
            alpha = max_eval;
        } else if (quiet && eval <= prev_alpha) {
            penalize_history(side, move, depth);
        }

        if (alpha >= beta) {
            // Store killer move
            if (ply < MAX_PLY && quiet) {
                if (killer_moves_list[ply][0] != move) {
                    killer_moves_list[ply][1] = killer_moves_list[ply][0];
                    killer_moves_list[ply][0] = move;
                }
                update_history(side, move, depth);
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
    store_tt(node->zobrist_key, depth, to_tt_score(max_eval, ply), bound,
        best_move_at_node);
    return max_eval;
}
