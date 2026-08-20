#include "engine/uci/uci.h"

#include "chess/perft/perft.h"
#include "engine/engine.h"
#include "engine/search/search.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief Advances a command cursor past leading whitespace.
 *
 * Leaves the cursor at the first non-whitespace character or NUL.
 *
 * @param str Address of the cursor to advance.
 */
static void skip_whitespace(const char** str)
{
    while (**str && isspace((unsigned char)**str)) {
        (*str)++;
    }
}

/**
 * @brief Reads one whitespace-delimited token and advances the command cursor.
 *
 * Long tokens are truncated to fit @p out.
 *
 * @param command Address of the cursor to read and advance.
 * @param out Destination buffer for a NUL-terminated token.
 * @param out_size Capacity of @p out, which must be greater than zero.
 * @return true when a token was read, or false when only whitespace remains.
 */
static bool next_token(const char** command, char* out, size_t out_size)
{
    skip_whitespace(command);
    if (**command == '\0') {
        return false;
    }

    size_t len = strcspn(*command, " \t\r\n");
    if (len >= out_size) {
        len = out_size - 1;
    }

    strncpy(out, *command, len);
    out[len] = '\0';
    *command += strcspn(*command, " \t\r\n");
    return true;
}

/**
 * @brief Reports whether a token starts a recognized `go` argument.
 *
 * Used to end a `searchmoves` list. Keep it in sync with handle_go_command().
 *
 * @param token NUL-terminated token to classify.
 * @return true when @p token is a supported `go` keyword.
 */
static bool is_go_keyword(const char* token)
{
    return !strcmp(token, "searchmoves") || !strcmp(token, "ponder") || !strcmp(token, "wtime")
        || !strcmp(token, "btime") || !strcmp(token, "winc") || !strcmp(token, "binc")
        || !strcmp(token, "movestogo") || !strcmp(token, "depth") || !strcmp(token, "nodes")
        || !strcmp(token, "mate") || !strcmp(token, "movetime") || !strcmp(token, "infinite");
}

/**
 * @brief Reads the next `go` numeric argument as an integer.
 *
 * Missing values fail; malformed values follow atoi() semantics and become
 * zero.
 *
 * @param command Address of the cursor to read and advance.
 * @param out Destination for the parsed value.
 * @return false if no token is present; true after storing the atoi() result.
 */
static bool parse_next_int_token(const char** command, int* out)
{
    char token[64];
    if (!next_token(command, token, sizeof(token))) {
        return false;
    }

    *out = atoi(token);
    return true;
}

/**
 * @brief Applies one supported UCI `setoption` command.
 *
 * Supports `Hash`, `Clear Hash`, and `MultiPV`.
 *
 * @param command Remaining text from a `setoption` command.
 */
static void handle_set_option_command(const char* command)
{
    char token[64];
    char option[64] = "";
    if (!next_token(&command, token, sizeof(token)) || strcmp(token, "name")
        || !next_token(&command, option, sizeof(option))) {
        printf("info string Unsupported option: (none)\n");
        fflush(stdout);
        return;
    }

    if (!strcmp(option, "Hash")) {
        char value[64];
        if (!next_token(&command, token, sizeof(token)) || strcmp(token, "value")
            || !next_token(&command, value, sizeof(value))) {
            printf("info string setoption Hash requires a value\n");
            fflush(stdout);
            return;
        }

        char* end_ptr = NULL;
        errno         = 0;
        long mb       = strtol(value, &end_ptr, 10);
        if (errno != 0 || end_ptr == value || *end_ptr != '\0') {
            printf("info string Invalid Hash value: %s\n", value);
            fflush(stdout);
            return;
        }

        long applied_mb = mb;
        if (!engine_set_hash_mb(mb, &applied_mb)) {
            printf("info string Failed to set Hash value\n");
            fflush(stdout);
            return;
        }

        printf("info string Hash set to %ld MB\n", applied_mb);
        fflush(stdout);
        return;
    }

    if (!strcmp(option, "MultiPV")) {
        char value[64];
        if (!next_token(&command, token, sizeof(token)) || strcmp(token, "value")
            || !next_token(&command, value, sizeof(value))) {
            printf("info string setoption MultiPV requires a value\n");
            fflush(stdout);
            return;
        }

        char* end_ptr = NULL;
        errno         = 0;
        long requested = strtol(value, &end_ptr, 10);
        if (errno != 0 || end_ptr == value || *end_ptr != '\0') {
            printf("info string Invalid MultiPV value: %s\n", value);
            fflush(stdout);
            return;
        }

        int requested_multipv = requested < 1 ? 1
            : requested > MAX_LEGAL_MOVES ? MAX_LEGAL_MOVES : (int)requested;
        int applied_multipv = 1;
        if (!engine_set_multipv(requested_multipv, &applied_multipv)) {
            printf("info string Failed to set MultiPV value\n");
            fflush(stdout);
            return;
        }

        printf("info string MultiPV set to %d\n", applied_multipv);
        fflush(stdout);
        return;
    }

    if (!strcmp(option, "Clear") && next_token(&command, token, sizeof(token))
        && !strcmp(token, "Hash")) {
        engine_clear_hash();
        printf("info string Hash cleared\n");
        fflush(stdout);
        return;
    }

    printf("info string Unsupported option: %s\n", option);
    fflush(stdout);
}

/**
 * @brief Reads one CRLF- or LF-terminated UCI input line from standard input.
 *
 * Reuses and grows the caller-owned buffer. Returns false on EOF or allocation
 * failure.
 *
 * @param line_input Address of the reusable, heap-allocated input buffer.
 * @param capacity Address of that buffer's allocated capacity.
 * @return true with a NUL-terminated line in @p line_input, otherwise false.
 */
static bool safe_line_read(char** line_input, size_t* capacity)
{
    if (*line_input == NULL || *capacity == 0) {
        *capacity   = 256;
        *line_input = malloc(*capacity);
        if (*line_input == NULL) {
            return false;
        }
    }

    size_t len = 0;
    int ch     = EOF;
    while ((ch = fgetc(stdin)) != EOF) {
        if (ch == '\n') {
            break;
        }
        if (ch == '\r') {
            continue;
        }

        if (len + 1 >= *capacity) {
            size_t new_capacity  = *capacity * 2;
            char* new_line_input = realloc(*line_input, new_capacity);
            if (new_line_input == NULL) {
                return false;
            }
            *line_input = new_line_input;
            *capacity   = new_capacity;
        }

        (*line_input)[len++] = (char)ch;
    }

    if (ch == EOF && len == 0) {
        return false;
    }

    (*line_input)[len] = '\0';
    return true;
}

/**
 * @brief Parses a UCI `go` command and starts the requested search.
 *
 * `searchmoves` is validated against the current board. Invalid moves are
 * ignored; an empty requested list emits `bestmove 0000`. Unknown fields are
 * ignored and numeric fields use atoi()-style parsing.
 *
 * @param command Remaining text from a `go` command.
 */
void handle_go_command(const char* command)
{
    SearchLimits go_cmd         = { 0 };
    go_cmd.multipv               = engine_get_multipv();
    bool search_moves_specified = false;
    CBoard current_board;
    bool have_current_board = engine_copy_board(&current_board);

    char token[64];
    while (next_token(&command, token, sizeof(token))) {
        if (!strcmp(token, "searchmoves")) {
            search_moves_specified = true;
            while (1) {
                const char* saved = command;
                char moveToken[32];
                if (!next_token(&command, moveToken, sizeof(moveToken))) {
                    break;
                }

                if (is_go_keyword(moveToken)) {
                    command = saved;
                    break;
                }

                char error_buf[128] = "";
                Move move           = have_current_board
                    ? move_from_uci_string(&current_board, moveToken, error_buf, sizeof(error_buf))
                    : MOVE_NONE;

                if (move != MOVE_NONE) {
                    if (go_cmd.search_moves.count < 256) {
                        go_cmd.search_moves.moves[go_cmd.search_moves.count++] = move;
                    }
                } else if (engine_is_debug_mode()) {
                    printf("info string Invalid move in go "
                           "searchmoves: %s (%s)\n",
                           moveToken, error_buf[0] ? error_buf : "invalid");
                }
            }
        } else if (!strcmp(token, "ponder")) {
            go_cmd.ponder = true;
        } else if (!strcmp(token, "infinite")) {
            go_cmd.infinite_search = true;
        } else if (!strcmp(token, "wtime")) {
            parse_next_int_token(&command, &go_cmd.time_for_white_ms);
        } else if (!strcmp(token, "btime")) {
            parse_next_int_token(&command, &go_cmd.time_for_black_ms);
        } else if (!strcmp(token, "winc")) {
            parse_next_int_token(&command, &go_cmd.increment_for_white_ms);
        } else if (!strcmp(token, "binc")) {
            parse_next_int_token(&command, &go_cmd.increment_for_black_ms);
        } else if (!strcmp(token, "movestogo")) {
            parse_next_int_token(&command, &go_cmd.moves_until_next_time_control);
        } else if (!strcmp(token, "depth")) {
            parse_next_int_token(&command, &go_cmd.depth_limit);
        } else if (!strcmp(token, "nodes")) {
            parse_next_int_token(&command, &go_cmd.node_limit);
        } else if (!strcmp(token, "mate")) {
            parse_next_int_token(&command, &go_cmd.search_for_mate_in_n_moves);
        } else if (!strcmp(token, "movetime")) {
            parse_next_int_token(&command, &go_cmd.time_limit_ms);
        } else if (engine_is_debug_mode()) {
            printf("info string Ignoring unknown go token: %s\n", token);
        }
    }

    if (search_moves_specified && go_cmd.search_moves.count == 0) {
        printf("bestmove 0000\n");
        fflush(stdout);
        return;
    }

    // handle go command with the parsed parameters in goCmd struct
    if (engine_is_debug_mode()) {
        printf("info string Parsed go command parameters:\n");
        printf("  ponder: %d\n", go_cmd.ponder);
        printf("  infinite_search: %d\n", go_cmd.infinite_search);
        printf("  time_for_white_ms: %d\n", go_cmd.time_for_white_ms);
        printf("  time_for_black_ms: %d\n", go_cmd.time_for_black_ms);
        printf("  increment_for_white_ms: %d\n", go_cmd.increment_for_white_ms);
        printf("  increment_for_black_ms: %d\n", go_cmd.increment_for_black_ms);
        printf("  moves_until_next_time_control: %d\n", go_cmd.moves_until_next_time_control);
        printf("  depth_limit: %d\n", go_cmd.depth_limit);
        printf("  node_limit: %d\n", go_cmd.node_limit);
        printf("  search_for_mate_in_n_moves: %d\n", go_cmd.search_for_mate_in_n_moves);
        printf("  time_limit_ms: %d\n", go_cmd.time_limit_ms);
        printf("  multipv: %d\n", go_cmd.multipv);
        printf("  search_moves count: %d\n", go_cmd.search_moves.count);
    }

    char error_buf[128] = "";
    if (!engine_start_search(&go_cmd, error_buf, sizeof(error_buf))) {
        printf("info string %s\n", error_buf[0] ? error_buf : "failed to start search");
        fflush(stdout);
    }
}

/**
 * @brief Runs Prophet's non-standard perft command on the current position.
 *
 * Supports `perft <depth>` and `perft suite`; stops an active search first.
 * NPS uses CPU time and is for development only.
 *
 * @param command Remaining text from a `perft` command.
 */
static void handle_perft_command(const char* command)
{
    engine_stop_search();

    skip_whitespace(&command);

    // check for "suite" keyword for running all perft tests defined
    // in perft.h
    if (!strncmp(command, "suite", 5)
        && (command[5] == '\0' || isspace((unsigned char)command[5]))) {
        command += 5;
        skip_whitespace(&command);
        run_perft_test_suite();
        return;
    }

    int max_depth = 0;
    if (!parse_next_int_token(&command, &max_depth) || max_depth <= 0) {
        printf("info string Usage: perft <max_depth> (run perft on "
               "current "
               "position to <max_depth>) or perft suite (run all "
               "perft tests, "
               "predefined)\n");
        fflush(stdout);
        return;
    }

    CBoard base_board;
    if (!engine_copy_board(&base_board)) {
        printf("info string Failed to copy board for perft\n");
        fflush(stdout);
        return;
    }

    for (int depth = 1; depth <= max_depth; depth++) {
        CBoard board   = base_board;
        clock_t start  = clock();
        uint64_t nodes = perft(&board, depth);
        clock_t end    = clock();
        double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
        double nps     = elapsed > 0 ? nodes / elapsed : 0;

        printf("perft depth %d nodes %" PRIu64 " nps %.0f time %.3f\n", depth, nodes, nps, elapsed);
        fflush(stdout);
    }
}
/**
 * @brief Replaces the engine position and optionally plays a UCI move list.
 *
 * Accepts `startpos` or `fen`, optionally followed by `moves`, and stops an
 * active search before changing the board. Invalid moves are reported but do
 * not abort the remaining list.
 *
 * @param command Remaining text from a `position` command.
 */
void handle_position_command(const char* command)
{
    engine_stop_search();

    skip_whitespace(&command);

    if (!strncmp(command, "startpos", 8)
        && (command[8] == '\0' || isspace((unsigned char)command[8]))) {
        bool success = engine_set_position_fen(START_FEN);
        if (!success) {
            printf("info string Failed to parse FEN: %s\n", START_FEN);
            fflush(stdout);
            return;
        }
        command += 8;
    } else if (!strncmp(command, "fen", 3)
               && (command[3] == '\0' || isspace((unsigned char)command[3]))) {
        command += 3;
        skip_whitespace(&command);

        // Find optional " moves " token as a full keyword, not as a
        // set of chars.
        const char* moves_pos = strstr(command, " moves");
        size_t fen_length     = moves_pos ? (size_t)(moves_pos - command) : strlen(command);
        while (fen_length > 0 && isspace((unsigned char)command[fen_length - 1])) {
            fen_length--;
        }

        char fen_copy[1024];
        if (fen_length >= sizeof(fen_copy)) {
            printf("info string FEN too long in position command\n");
            return;
        }
        strncpy(fen_copy, command, fen_length);
        fen_copy[fen_length] = '\0';
        bool success         = engine_set_position_fen(fen_copy);
        if (!success) {
            printf("info string Failed to parse FEN: %s\n", fen_copy);
            fflush(stdout);
            return;
        }
        command += fen_length;

        if (engine_is_debug_mode()) {
            printf("info string Parsed FEN: %s\n", fen_copy);
            printf("info string Command after FEN: '%s'\n", command);
        }
    }

    // process moves if present
    skip_whitespace(&command);
    if (!strncmp(command, "moves", 5)
        && (command[5] == '\0' || isspace((unsigned char)command[5]))) {
        command += 5;
        skip_whitespace(&command);
        size_t move_length = strcspn(command, " \r\n");
        while (move_length > 0) {
            char algebraic_move_str[16];

            if (move_length >= sizeof(algebraic_move_str)) {
                printf("info string Move token too long in position "
                       "command\n");
                return;
            }

            strncpy(algebraic_move_str, command, move_length);
            algebraic_move_str[move_length] = '\0';
            char error_buf[128]             = "";
            if (!engine_apply_uci_move(algebraic_move_str, error_buf, sizeof(error_buf))) {
                printf("info string error: %s, making move %s\n", error_buf, algebraic_move_str);
                fflush(stdout);
            }
            command += move_length;
            skip_whitespace(&command);
            move_length = strcspn(command, " \r\n");
        }
    }
}
/**
 * @brief Runs the stdin/stdout UCI command loop until `quit` or input EOF.
 *
 * Commands are dispatched serially while searches run on the engine thread.
 * `isready` responds immediately; `quit` shuts down the engine.
 */
void uci_loop(void)
{
    char* line_input     = NULL;
    size_t line_capacity = 0;

    while (safe_line_read(&line_input, &line_capacity)) {
        const char* p = line_input;
        char command[32];
        if (!next_token(&p, command, sizeof(command))) {
            continue; // continue if input is empty or only whitespace
        }
        if (!strcmp(command, "quit")) {
            engine_shutdown();
            break;
        }

        else if (!strcmp(command, "isready")) {
            printf("readyok\n");
            fflush(stdout);
        } else if (!strcmp(command, "uci")) {
            engine_init();

            printf("id name Prophet dev\n"); // TODO: figure out how
            // to put actual version
            // info here
            printf("id author Nicolas Carbone\n");
            printf("option name Hash type spin default 64 min 1 max "
                   "1024\n");
            printf("option name Clear Hash type button\n");
            printf("option name MultiPV type spin default 1 min 1 max 256\n");
            printf("uciok\n");
            fflush(stdout);
        } else if (!strcmp(command, "setoption")) {
            handle_set_option_command(p);
        } else if (!strcmp(command, "ucinewgame")) {
            engine_new_game();
        } else if (!strcmp(command, "position")) {
            handle_position_command(p);
        } else if (!strcmp(command, "go")) {
            handle_go_command(p);
        } else if (!strcmp(command, "perft")) {
            handle_perft_command(p);
        } else if (!strcmp(command, "stop")) {
            engine_stop_search();
        } else if (!strcmp(command, "ponderhit")) {
            engine_handle_ponder_hit();
            if (engine_is_debug_mode()) {
                printf("info string ponderhit received\n");
            }
        } else if (!strcmp(command, "debug")) {
            char value[16] = "";
            if (next_token(&p, value, sizeof(value)) && !strcmp(value, "on")) {
                engine_set_debug_mode(true);
                printf("info string Debug mode enabled\n");
            } else if (!strcmp(value, "off")) {
                engine_set_debug_mode(false);
                printf("info string Debug mode disabled\n");
            } else {
                printf("info string Invalid debug command: %s\n", p);
            }
        } else if (!strcmp(command, "printboard")) {
            if (engine_is_debug_mode()) {
                engine_print_board();
                fflush(stdout);
            } else {
                printf("info string 'printboard' command only works "
                       "in debug "
                       "mode\n");
                fflush(stdout);
            }
        } else {
            if (engine_is_debug_mode()) {
                printf("info string Ignoring unknown command: %s\n", command);
                fflush(stdout);
            }
        }
    }

    free(line_input);
}
