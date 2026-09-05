#include "chess/board/cboard.h"
#include "chess/board/zobrist.h"
#include "chess/movegen/sliding_attacks.h"
#include "engine/eval/hceval.h"
#include "engine/search/search.h"
#include "engine/tt/tt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SearchResult run_search(const char* fen, int depth_limit, int node_limit,
                               bool stop_before_search)
{
    CBoard board;
    if (!fen_string_to_cboard(fen, &board)) {
        fprintf(stderr, "failed to parse test FEN: %s\n", fen);
        return (SearchResult) { .best_move = MOVE_NONE };
    }

    SearchLimits limits = { 0 };
    limits.depth_limit  = depth_limit;
    limits.node_limit   = node_limit;
    limits.multipv      = 1;

    SearchInput input   = {
        .board               = board,
        .position_history    = { .keys = { board.zobrist_key }, .count = 1 },
        .limits              = limits,
        .suppress_uci_output = true,
    };

    clear_tt();
    SearchControl control;
    search_control_reset(&control, false);
    if (stop_before_search) {
        search_request_stop(&control);
    }
    return search_run(&input, &control);
}

static int test_fixed_depth_result(void)
{
    SearchResult result = run_search(START_FEN, 2, 0, false);
    if (!result.completed || result.completed_depth != 2 || result.best_move == MOVE_NONE
        || result.root_line_count != 1 || result.root_lines[0].move != result.best_move) {
        fprintf(stderr, "fixed-depth result is incomplete\n");
        return 1;
    }
    return 0;
}

static int test_partial_iteration_preserves_result(void)
{
    SearchResult completed = run_search(START_FEN, 1, 0, false);
    if (!completed.completed || completed.completed_depth != 1
        || completed.best_move == MOVE_NONE) {
        fprintf(stderr, "failed to produce baseline completed result\n");
        return 1;
    }

    SearchResult partial = run_search(START_FEN, 3, (int)completed.nodes + 1, false);
    if (partial.completed || partial.completed_depth != 1
        || partial.best_move != completed.best_move || partial.score != completed.score
        || partial.root_line_count != completed.root_line_count
        || partial.root_lines[0].move != completed.root_lines[0].move
        || partial.root_lines[0].score != completed.root_lines[0].score) {
        fprintf(stderr, "partial iteration replaced the last completed result\n");
        return 1;
    }
    return 0;
}

static int test_stop_before_search(void)
{
    SearchResult result = run_search(START_FEN, 2, 0, true);
    if (result.completed || result.completed_depth != 0 || result.best_move != MOVE_NONE
        || result.score != 0) {
        fprintf(stderr, "pre-stopped search returned a result\n");
        return 1;
    }
    return 0;
}

static int test_terminal_and_draw_results(void)
{
    SearchResult checkmate = run_search("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1", 1, 0, false);
    if (!checkmate.completed || checkmate.completed_depth != 1 || checkmate.best_move != MOVE_NONE
        || checkmate.score != -MATE_SCORE) {
        fprintf(stderr, "checkmate result is incorrect\n");
        return 1;
    }

    SearchResult stalemate = run_search("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", 1, 0, false);
    if (!stalemate.completed || stalemate.completed_depth != 1 || stalemate.best_move != MOVE_NONE
        || stalemate.score != 0) {
        fprintf(stderr, "stalemate result is incorrect\n");
        return 1;
    }

    SearchResult draw = run_search("7k/8/8/8/8/8/6K1/8 w - - 150 1", 1, 0, false);
    if (!draw.completed || draw.completed_depth != 1 || draw.best_move == MOVE_NONE
        || draw.score != 0) {
        fprintf(stderr, "draw result is incorrect\n");
        return 1;
    }
    return 0;
}

static int test_black_root_score_orientation(void)
{
    SearchResult black_to_move = run_search("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1", 1, 0, false);
    SearchResult white_to_move = run_search("7k/5Q2/6K1/8/8/8/8/8 w - - 0 1", 1, 0, false);

    if (black_to_move.score >= 0 || white_to_move.score <= 0) {
        fprintf(stderr, "root scores are not side-to-move relative\n");
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    init_sliding_attacks();
    init_zobrist_keys();
    hc_eval_init();
    init_tt(1);

    if (argc == 4 && strcmp(argv[1], "--probe") == 0) {
        SearchResult result = run_search(argv[2], atoi(argv[3]), 0, false);
        char move[6];
        move_to_uci_string(result.best_move, move);
        printf("%s %d %d %d %d %d\n", move, result.score, result.completed_depth, result.completed,
               MATE_SCORE, MATE_THRESHOLD);
        free_tt();
        return 0;
    }
    if (argc != 1) {
        free_tt();
        return 2;
    }

    int failed = test_fixed_depth_result() || test_partial_iteration_preserves_result()
        || test_stop_before_search() || test_terminal_and_draw_results()
        || test_black_root_score_orientation();

    free_tt();
    if (failed) {
        return 1;
    }

    printf("Search result API tests passed\n");
    return 0;
}
