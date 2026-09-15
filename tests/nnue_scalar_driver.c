#include "chess/board/cboard.h"
#include "chess/board/zobrist.h"
#include "chess/movegen/move.h"
#include "chess/movegen/move_make.h"
#include "chess/movegen/movegen.h"
#include "engine/eval/nnue.h"

#include <stdio.h>
#include <string.h>

static int check_position(const char* fen, const char* const* moves, size_t move_count)
{
    CBoard board;
    NnueAccumulator accumulator;
    if (!fen_string_to_cboard(fen, &board) || !nnue_refresh_accumulator(&board, &accumulator)) {
        return 1;
    }
    for (size_t index = 0; index < move_count; index++) {
        char error[64] = "";
        Move move      = move_from_uci_string(&board, moves[index], error, sizeof(error));
        CBoard parent  = board;
        NnueAccumulator parent_accumulator = accumulator;
        if (move == MOVE_NONE) {
            fprintf(stderr, "invalid test move %s: %s\n", moves[index], error);
            return 1;
        }
        UndoInfo undo = make_move(&board, move);
        NnueAccumulator incremental;
        NnueAccumulator refreshed;
        if (!nnue_update_accumulator(&parent, &board, &parent_accumulator, &incremental)
            || !nnue_refresh_accumulator(&board, &refreshed)
            || memcmp(&incremental, &refreshed, sizeof(refreshed))
            || nnue_evaluate_accumulator(&board, &incremental) != nnue_evaluate_cboard(&board)) {
            fprintf(stderr, "incremental mismatch after %s\n", moves[index]);
            return 1;
        }
        unmake_move(&board, move, undo);
        if (memcmp(&board, &parent, sizeof(board))
            || memcmp(&parent_accumulator, &accumulator, sizeof(accumulator))) {
            fprintf(stderr, "parent changed after unmake %s\n", moves[index]);
            return 1;
        }
        make_move(&board, move);
        accumulator = incremental;
    }
    return 0;
}

static int check_incremental(void)
{
    static const char* opening[]           = { "e2e4", "d7d5", "e4d5", "g8f6", "g1f3" };
    static const char* castle[]            = { "e1g1", "e8c8" };
    static const char* ep[]                = { "e5d6" };
    static const char* promotion[]         = { "a7a8q" };
    static const char* promotion_capture[] = { "a7b8q" };
    static const char* king_move[]         = { "d4e4" };
    int result = check_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", opening,
                                sizeof(opening) / sizeof(opening[0]))
        || check_position("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", castle,
                          sizeof(castle) / sizeof(castle[0]))
        || check_position("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", ep, sizeof(ep) / sizeof(ep[0]))
        || check_position("4k3/P7/8/8/8/8/8/4K3 w - - 0 1", promotion,
                          sizeof(promotion) / sizeof(promotion[0]))
        || check_position("1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", promotion_capture,
                          sizeof(promotion_capture) / sizeof(promotion_capture[0]))
        || check_position("7k/8/8/8/3K4/8/8/8 w - - 0 1", king_move,
                          sizeof(king_move) / sizeof(king_move[0]));
    if (result) {
        return result;
    }

    CBoard board;
    NnueAccumulator accumulator;
    if (!fen_string_to_cboard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &board)
        || !nnue_refresh_accumulator(&board, &accumulator)) {
        return 1;
    }
    CBoard null_child       = board;
    null_child.side_to_move = color_opposite(null_child.side_to_move);
    null_child.ep_square    = NO_SQUARE;
    NnueAccumulator null_accumulator;
    NnueAccumulator null_refreshed;
    if (!nnue_update_accumulator(&board, &null_child, &accumulator, &null_accumulator)
        || !nnue_refresh_accumulator(&null_child, &null_refreshed)
        || memcmp(&null_accumulator, &null_refreshed, sizeof(null_refreshed))) {
        fprintf(stderr, "null move accumulator mismatch\n");
        return 1;
    }

    uint32_t state = 0x9e3779b9U;
    for (int game = 0; game < 2; game++) {
        if (!fen_string_to_cboard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                                  &board)
            || !nnue_refresh_accumulator(&board, &accumulator)) {
            return 1;
        }
        for (int ply = 0; ply < 64; ply++) {
            MoveList moves;
            init_move_list(&moves);
            generate_legal_moves(&board, &moves);
            if (moves.count == 0) {
                break;
            }
            state                              = state * 1664525U + 1013904223U;
            Move move                          = moves.moves[state % (uint32_t)moves.count];
            CBoard parent                      = board;
            NnueAccumulator parent_accumulator = accumulator;
            UndoInfo undo                      = make_move(&board, move);
            NnueAccumulator incremental;
            NnueAccumulator refreshed;
            if (!nnue_update_accumulator(&parent, &board, &parent_accumulator, &incremental)
                || !nnue_refresh_accumulator(&board, &refreshed)
                || memcmp(&incremental, &refreshed, sizeof(refreshed))
                || nnue_evaluate_accumulator(&board, &incremental)
                    != nnue_evaluate_cboard(&board)) {
                fprintf(stderr, "seeded incremental mismatch at ply %d\n", ply);
                return 1;
            }
            unmake_move(&board, move, undo);
            if (memcmp(&board, &parent, sizeof(board))
                || memcmp(&accumulator, &parent_accumulator, sizeof(accumulator))) {
                fprintf(stderr, "seeded parent changed after unmake\n");
                return 1;
            }
            make_move(&board, move);
            accumulator = incremental;
        }
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        return 2;
    }
    init_zobrist_keys();
    char error[128] = "";
    if (!nnue_load_file(argv[1], error, sizeof(error))) {
        fprintf(stderr, "%s\n", error);
        return 2;
    }
    if (argc == 3 && !strcmp(argv[2], "--incremental")) {
        return check_incremental();
    }
    for (int index = 2; index < argc; index++) {
        CBoard board;
        if (!fen_string_to_cboard(argv[index], &board)) {
            fprintf(stderr, "invalid FEN\n");
            return 2;
        }
        printf("%d\n", nnue_evaluate_cboard(&board));
    }
    return 0;
}
