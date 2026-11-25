#include "chess/board/cboard.h"
#include "chess/board/zobrist.h"
#include "chess/core/bitboard.h"
#include "chess/movegen/move.h"
#include "chess/movegen/move_make.h"
#include "chess/movegen/movegen.h"
#include "engine/eval/nnue.h"

#include <stdio.h>
#include <string.h>

/** Check the record against board differences, independently of NNUE feature mapping. */
static bool check_move_changes(const CBoard* parent, const CBoard* child, Move move,
                               UndoInfo undo, const MoveChanges* changes)
{
    if (changes->removed_count > 2 || changes->added_count > 2) {
        return false;
    }
    for (Color color = WHITE; color <= BLACK; color++) {
        for (PieceType piece = PAWN; piece <= KING; piece++) {
            Bitboard removed = parent->piece_bbs[color][piece] & ~child->piece_bbs[color][piece];
            Bitboard added = child->piece_bbs[color][piece] & ~parent->piece_bbs[color][piece];
            for (int list = 0; list < 2; list++) {
                const PieceChange* entries = list ? changes->added : changes->removed;
                uint8_t count = list ? changes->added_count : changes->removed_count;
                Bitboard* remaining = list ? &added : &removed;
                for (uint8_t index = 0; index < count; index++) {
                    const PieceChange* entry = &entries[index];
                    if (entry->color > BLACK || entry->piece < PAWN || entry->piece > KING
                        || entry->square >= NO_SQUARE) {
                        return false;
                    }
                    if (entry->color == color && entry->piece == piece) {
                        if (!bitboard_is_bit_set(*remaining, entry->square)) {
                            return false;
                        }
                        bitboard_clear_square_bit(remaining, entry->square);
                    }
                }
            }
            if (removed || added) {
                return false;
            }
        }
    }
    CBoard plain = *parent;
    UndoInfo plain_undo = make_move(&plain, move, NULL);
    return !memcmp(&plain, child, sizeof(plain))
        && plain_undo.captured_piecetype == undo.captured_piecetype
        && plain_undo.captured_square == undo.captured_square
        && plain_undo.previous_ep_square == undo.previous_ep_square
        && plain_undo.previous_halfmove_clock == undo.previous_halfmove_clock
        && plain_undo.previous_castling_rights == undo.previous_castling_rights
        && plain_undo.previous_zobrist_key == undo.previous_zobrist_key;
}

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
        MoveChanges changes = { .removed_count = 2, .added_count = 2 };
        UndoInfo undo = make_move(&board, move, &changes);
        NnueAccumulator incremental;
        NnueAccumulator refreshed;
        if (!check_move_changes(&parent, &board, move, undo, &changes)
            || !nnue_update_accumulator(&board, &changes, &parent_accumulator, &incremental)
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
        make_move(&board, move, NULL);
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

    static const struct {
        const char* fen;
        const char* move;
    } special_moves[] = {
        { "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1c1" },
        { "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", "e8g8" },
        { "4k3/8/8/8/3Pp3/8/8/4K3 b - d3 0 1", "e4d3" },
        { "7k/8/8/8/3Kp3/8/8/8 w - - 0 1", "d4e4" },
        { "8/8/8/3kp3/8/8/8/7K b - - 0 1", "d5c5" },
    };
    for (size_t index = 0; index < sizeof(special_moves) / sizeof(special_moves[0]); index++) {
        if (check_position(special_moves[index].fen, &special_moves[index].move, 1)) {
            return 1;
        }
    }
    // Cover every promoted type, with and without captures, for both colors.
    for (size_t index = 0; index < 4; index++) {
        char white_quiet[] = "a7a8q", white_capture[] = "a7b8q";
        char black_quiet[] = "a2a1q", black_capture[] = "a2b1q";
        white_quiet[4] = white_capture[4] = black_quiet[4] = black_capture[4] = "qrbn"[index];
        const char* moves[] = { white_quiet, white_capture, black_quiet, black_capture };
        if (check_position("4k3/P7/8/8/8/8/8/4K3 w - - 0 1", &moves[0], 1)
            || check_position("1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", &moves[1], 1)
            || check_position("4k3/8/8/8/8/8/p7/4K3 b - - 0 1", &moves[2], 1)
            || check_position("4k3/8/8/8/8/8/p7/1R2K3 b - - 0 1", &moves[3], 1)) {
            return 1;
        }
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
    MoveChanges no_changes = { 0 };
    NnueAccumulator null_accumulator;
    NnueAccumulator null_refreshed;
    if (!nnue_update_accumulator(&null_child, &no_changes, &accumulator, &null_accumulator)
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
            MoveChanges changes = { .removed_count = 2, .added_count = 2 };
            UndoInfo undo = make_move(&board, move, &changes);
            NnueAccumulator incremental;
            NnueAccumulator refreshed;
            if (!check_move_changes(&parent, &board, move, undo, &changes)
                || !nnue_update_accumulator(&board, &changes, &parent_accumulator, &incremental)
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
            make_move(&board, move, NULL);
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
