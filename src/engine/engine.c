#include "engine/engine.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attacks/sliding_attacks.h"
#include "board/zobrist.h"
#include "eval/hceval.h"
#include "movegen/move_make.h"
#include "movegen/movegen.h"
#include "search/search.h"
#include "search/tt.h"

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

// Define the global atomic flags here
atomic_bool search_stop_flag = false;
atomic_bool search_is_pondering = false;

static void set_error(char* error_buf, size_t error_buf_size, const char* message)
{
    if (!error_buf || error_buf_size == 0) {
        return;
    }
    snprintf(error_buf, error_buf_size, "%s", message);
}

static Square algebraic_notation_to_square(const char* algebraic_square_str)
{
    if (strlen(algebraic_square_str) < 2) {
        return NO_SQUARE;
    }
    char file = algebraic_square_str[0];
    char rank = algebraic_square_str[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
        return NO_SQUARE;
    }
    int file_idx = file - 'a';
    int rank_idx = rank - '1';
    return (Square)(rank_idx * 8 + file_idx);
}

// Engine state (owned and managed by the engine module).
static EngineState engine_state = { 0 };

void engine_init(void)
{
    // all of these functions are idempotent
    init_sliding_attacks();
    init_zobrist_keys();
    hc_eval_init();
    init_tt(64); // 64 MB TT by default, can be overridden by UCI option later
    atomic_store(&search_stop_flag, false);
    atomic_store(&search_is_pondering, false);

    engine_state.is_searching = false;
    engine_state.is_debug_mode = false;
    engine_set_position_startpos();
}

void engine_shutdown(void)
{
    engine_stop_search();
}

void engine_stop_search(void)
{
    if (!engine_state.is_searching) {
        return;
    }

    atomic_store(&search_stop_flag, true);
    pthread_join(engine_state.search_thread, NULL);
    engine_state.is_searching = false;
    atomic_store(&search_is_pondering, false);
}

bool engine_set_position_startpos(void)
{
    return fen_string_to_cboard(START_FEN, &engine_state.board);
}

bool engine_set_position_fen(const char* fen)
{
    if (!fen) {
        return false;
    }
    return fen_string_to_cboard(fen, &engine_state.board);
}

bool engine_parse_uci_move(const char* move_str, Move* out_move,
    char* error_buf, size_t error_buf_size)
{
    if (!move_str || !out_move) {
        set_error(error_buf, error_buf_size, "Invalid move input");
        return false;
    }

    if (strlen(move_str) < 4) {
        set_error(error_buf, error_buf_size, "Invalid move format");
        return false;
    }

    Square from = algebraic_notation_to_square(move_str);
    Square to = algebraic_notation_to_square(move_str + 2);
    if (from == NO_SQUARE || to == NO_SQUARE) {
        set_error(error_buf, error_buf_size, "Invalid move format");
        return false;
    }

    CBoard board_copy = engine_state.board;
    MoveList move_list;
    init_move_list(&move_list);
    generate_legal_moves(&board_copy, &move_list);

    char promotion_char = '\0';
    if (strlen(move_str) >= 5) {
        promotion_char = (char)tolower((unsigned char)move_str[4]);
    }

    for (int i = 0; i < move_list.count; i++) {
        if (move_get_from_square(move_list.moves[i]) == from && move_get_to_square(move_list.moves[i]) == to) {
            if (promotion_char == '\0') {
                *out_move = move_list.moves[i];
                return true;
            }

            if (!move_is_promotion(move_list.moves[i])) {
                continue;
            }

            PieceType promo_piecetype = move_get_promotion_piecetype(move_list.moves[i]);
            bool promotion_matches = (promotion_char == 'n' && promo_piecetype == KNIGHT)
                || (promotion_char == 'b' && promo_piecetype == BISHOP)
                || (promotion_char == 'r' && promo_piecetype == ROOK)
                || (promotion_char == 'q' && promo_piecetype == QUEEN);

            if (promotion_matches) {
                *out_move = move_list.moves[i];
                return true;
            }
        }
    }

    set_error(error_buf, error_buf_size, "Move not in legal moves list");
    return false;
}

bool engine_apply_uci_move(const char* move_str, char* error_buf,
    size_t error_buf_size)
{
    Move move = create_move(NO_SQUARE, NO_SQUARE, NORMAL, NO_PIECE);
    if (!engine_parse_uci_move(move_str, &move, error_buf, error_buf_size)) {
        return false;
    }

    if (move_get_from_square(move) == NO_SQUARE || move_get_to_square(move) == NO_SQUARE) {
        set_error(error_buf, error_buf_size, "Invalid move");
        return false;
    }

    make_move(&engine_state.board, move);
    return true;
}

void engine_new_game(void)
{
    engine_stop_search();
    engine_set_position_startpos();
    clear_tt();
    clear_search_heuristics();
}

bool engine_start_search(const SearchLimits* limits, char* error_buf,
    size_t error_buf_size)
{
    if (!limits) {
        set_error(error_buf, error_buf_size, "Search limits not provided");
        return false;
    }

    if (engine_state.is_searching) {
        engine_stop_search();
    }

    atomic_store(&search_stop_flag, false);
    atomic_store(&search_is_pondering, limits->ponder);
    engine_state.is_searching = true;

    SearchThreadData* thread_data = malloc(sizeof(SearchThreadData));
    if (thread_data == NULL) {
        set_error(error_buf, error_buf_size, "memory allocation failed");
        engine_state.is_searching = false;
        return false;
    }

    thread_data->board = engine_state.board;
    thread_data->search_limits = *limits;

    if (pthread_create(&engine_state.search_thread, NULL, search_worker, thread_data) != 0) {
        set_error(error_buf, error_buf_size, "failed to create search thread");
        free(thread_data);
        engine_state.is_searching = false;
        return false;
    }

    return true;
}

void engine_handle_ponder_hit(void)
{
    on_ponder_hit();
}

bool engine_set_hash_mb(long requested_mb, long* applied_mb)
{
    long mb = requested_mb;
    if (mb < 1) {
        mb = 1;
    } else if (mb > 1024) {
        mb = 1024;
    }

    engine_stop_search();
    init_tt((size_t)mb);

    if (applied_mb) {
        *applied_mb = mb;
    }

    return true;
}

void engine_clear_hash(void)
{
    engine_stop_search();
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
