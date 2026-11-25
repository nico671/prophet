// Include the implementation to test internal move filtering without a public test API.
#include "engine/search/search.c"
#include "chess/movegen/sliding_attacks.h"

static bool contains_move(const MoveList* moves, Move move)
{
    for (int i = 0; i < moves->count; i++) {
        if (moves->moves[i] == move) {
            return true;
        }
    }
    return false;
}

static int test_castling(void)
{
    static const struct {
        const char* fen;
        Square from;
        Square to;
        bool legal;
    } cases[] = {
        // White kingside: safe, attacked start, attacked transit, attacked destination.
        { "1k6/8/8/8/8/8/8/4K2R w K - 0 1", E1, G1, true },
        { "1k2r3/8/8/8/8/8/8/4K2R w K - 0 1", E1, G1, false },
        { "1k3r2/8/8/8/8/8/8/4K2R w K - 0 1", E1, G1, false },
        { "1k4r1/8/8/8/8/8/8/4K2R w K - 0 1", E1, G1, false },
        // White queenside.
        { "1k6/8/8/8/8/8/8/R3K3 w Q - 0 1", E1, C1, true },
        { "1k2r3/8/8/8/8/8/8/R3K3 w Q - 0 1", E1, C1, false },
        { "1k1r4/8/8/8/8/8/8/R3K3 w Q - 0 1", E1, C1, false },
        { "1kr5/8/8/8/8/8/8/R3K3 w Q - 0 1", E1, C1, false },
        // Black kingside.
        { "4k2r/8/8/8/8/8/8/1K6 b k - 0 1", E8, G8, true },
        { "4k2r/8/8/8/8/8/8/1K2R3 b k - 0 1", E8, G8, false },
        { "4k2r/8/8/8/8/8/8/1K3R2 b k - 0 1", E8, G8, false },
        { "4k2r/8/8/8/8/8/8/1K4R1 b k - 0 1", E8, G8, false },
        // Black queenside.
        { "r3k3/8/8/8/8/8/8/1K6 b q - 0 1", E8, C8, true },
        { "r3k3/8/8/8/8/8/8/1K2R3 b q - 0 1", E8, C8, false },
        { "r3k3/8/8/8/8/8/8/1K1R4 b q - 0 1", E8, C8, false },
        { "r3k3/8/8/8/8/8/8/1KR5 b q - 0 1", E8, C8, false },
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        CBoard board;
        if (!fen_string_to_cboard(cases[i].fen, &board)) {
            fprintf(stderr, "invalid castling test FEN: %s\n", cases[i].fen);
            return 1;
        }
        CBoard original = board;
        Move castle = create_move(cases[i].from, cases[i].to, CASTLE, NO_PIECE);
        MoveList moves;
        init_move_list(&moves);
        gen_all_pseudolegal_moves(&board, &moves);
        if (!contains_move(&moves, castle)) {
            fprintf(stderr, "castling case %zu does not exercise legality filtering\n", i);
            return 1;
        }
        init_move_list(&moves);
        generate_legal_moves(&board, &moves);
        if (contains_move(&moves, castle) != cases[i].legal) {
            fprintf(stderr, "legal generator failed castling case %zu\n", i);
            return 1;
        }

        SearchControl control;
        search_control_reset(&control, false);
        SearchContext ctx = { 0 };
        ctx.control = &control;
        ctx.position_history.keys[0] = board.zobrist_key;
        ctx.position_history.count = 1;

        clear_tt();
        // Order the castle first without supplying a reusable search score.
        store_tt(board.zobrist_key, 0, 0, TT_ALL, castle);
        // Any ordinary score cuts off. An accepted castle must be the stored move.
        negamax(&ctx, &board, 1, -MATE_SCORE, -MATE_SCORE + 1, board.side_to_move, 0);
        TTEntry* entry = probe_tt(board.zobrist_key);
        if (!entry || entry->depth != 1 || entry->bound != TT_CUT
            || (entry->best_move == castle) != cases[i].legal) {
            fprintf(stderr, "internal search failed castling case %zu: %s\n", i, cases[i].fen);
            return 1;
        }
        if (memcmp(&board, &original, sizeof(board)) != 0) {
            fprintf(stderr, "castling case %zu changed the board\n", i);
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    init_sliding_attacks();
    init_zobrist_keys();
    hc_eval_init();
    init_tt(1);
    int failed = test_castling();
    free_tt();
    if (failed) {
        return 1;
    }
    puts("Internal search castling tests passed (16 cases)");
    return 0;
}
