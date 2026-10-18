// Test quiescence directly without adding a public search test API.
#include "chess/movegen/sliding_attacks.h"
#include "engine/search/search.c"

static int check_quiescence(const char* fen, bool stalemate)
{
    CBoard board;
    if (!fen_string_to_cboard(fen, &board)) {
        fprintf(stderr, "invalid stalemate test FEN: %s\n", fen);
        return 1;
    }
    unsigned char original[sizeof(board)];
    memcpy(original, &board, sizeof(board));
    MoveList captures;
    init_move_list(&captures);
    generate_capture_moves(&board, &captures);
    if (is_king_in_check(&board, board.side_to_move) || captures.count != 0
        || has_legal_move(&board) == stalemate || memcmp(original, &board, sizeof(board)) != 0) {
        fprintf(stderr, "invalid stalemate/quiet-move fixture: %s\n", fen);
        return 1;
    }

    SearchControl control;
    search_control_reset(&control, false);
    SearchContext ctx            = { 0 };
    ctx.control                  = &control;
    ctx.position_history.keys[0] = board.zobrist_key;
    ctx.position_history.count   = 1;
    int stand_pat                = evaluate_node(&ctx, &board, 0);
    if (stand_pat == 0) {
        fprintf(stderr, "fixture must have a nonzero static score\n");
        return 1;
    }
    int expected = stalemate ? 0 : stand_pat;
    const struct {
        int alpha;
        int beta;
        int ply;
    } windows[] = {
        { -MATE_SCORE, MATE_SCORE, 0 },
        // The old stand-pat cutoff returned before generating any moves.
        { stand_pat - 2, stand_pat - 1, 0 },
        { -MATE_SCORE, MATE_SCORE, MAX_PLY - 1 },
    };
    for (size_t i = 0; i < sizeof(windows) / sizeof(windows[0]); i++) {
        clear_tt();
        int score = quiescence(&ctx, &board, windows[i].alpha, windows[i].beta, windows[i].ply);
        TTEntry* entry = probe_tt(board.zobrist_key);
        if (score != expected
            || (stalemate
                && (!entry || entry->score != 0 || entry->bound != TT_PV
                    || entry->best_move != MOVE_NONE))
            || memcmp(original, &board, sizeof(board)) != 0) {
            fprintf(stderr, "quiescence case %zu returned %d, expected %d: %s\n", i, score,
                    expected, fen);
            return 1;
        }
    }
    return 0;
}

static int check_forced_stalemate(void)
{
    CBoard board;
    if (!fen_string_to_cboard("7k/4Q3/6K1/8/8/8/8/8 w - - 0 1", &board)) {
        return 1;
    }
    Move move         = create_move(E7, F7, NORMAL, NO_PIECE);
    SearchInput input = {
        .board               = board,
        .position_history    = { .keys = { board.zobrist_key }, .count = 1 },
        .limits              = { .multipv = 1, .search_moves = { .moves = { move }, .count = 1 } },
        .suppress_uci_output = true,
    };
    for (int depth = 1; depth <= 2; depth++) {
        SearchControl control;
        search_control_reset(&control, false);
        input.limits.depth_limit = depth;
        clear_tt();
        SearchResult result = search_run(&input, &control);
        if (!result.completed || result.completed_depth != depth || result.best_move != move
            || result.score != 0) {
            fprintf(stderr, "forced stalemate at depth %d scored %d instead of zero\n", depth,
                    result.score);
            return 1;
        }
    }
    return 0;
}

static int check_checkmate(void)
{
    CBoard board;
    if (!fen_string_to_cboard("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1", &board)) {
        return 1;
    }
    SearchControl control;
    search_control_reset(&control, false);
    SearchContext ctx = { 0 };
    ctx.control       = &control;
    clear_tt();
    if (has_legal_move(&board)
        || quiescence(&ctx, &board, -MATE_SCORE, MATE_SCORE, 0) != -MATE_SCORE) {
        fprintf(stderr, "checkmate was not scored as mate\n");
        return 1;
    }
    return 0;
}

int main(void)
{
    init_sliding_attacks();
    init_zobrist_keys();
    hc_eval_init();
    init_tt(1);
    int failed = check_quiescence("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", true)
        || check_quiescence("8/8/8/8/8/6k1/5q2/7K w - - 0 1", true)
        || check_quiescence("7k/8/6K1/8/8/Q7/8/8 b - - 0 1", false) || check_checkmate()
        || check_forced_stalemate();
    free_tt();
    if (failed) {
        return 1;
    }
    puts("Quiescence stalemate tests passed");
    return 0;
}
