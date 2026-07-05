#include "engine/engine.h"
#include "perft/perft.h"
#include "uci/uci.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// standard starting position FEN string
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
void skip_whitespace(const char** str);

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

static bool is_go_keyword(const char* token)
{
    return !strcmp(token, "searchmoves") || !strcmp(token, "ponder")
        || !strcmp(token, "wtime") || !strcmp(token, "btime")
        || !strcmp(token, "winc") || !strcmp(token, "binc")
        || !strcmp(token, "movestogo") || !strcmp(token, "depth")
        || !strcmp(token, "nodes") || !strcmp(token, "mate")
        || !strcmp(token, "movetime") || !strcmp(token, "infinite");
}

static bool parse_next_int_token(const char** command, int* out)
{
    char token[64];
    if (!next_token(command, token, sizeof(token))) {
        return false;
    }

    *out = atoi(token);
    return true;
}

static bool equals_ignore_case(const char* a, const char* b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static void handle_set_option_command(const char* command)
{
    command += 9; // Skip "setoption"

    char token[64];
    char name[128]     = "";
    char value[128]    = "";
    bool reading_name  = false;
    bool reading_value = false;

    while (next_token(&command, token, sizeof(token))) {
        if (equals_ignore_case(token, "name")) {
            reading_name  = true;
            reading_value = false;
            name[0]       = '\0';
            continue;
        }

        if (equals_ignore_case(token, "value")) {
            reading_value = true;
            reading_name  = false;
            value[0]      = '\0';
            continue;
        }

        if (reading_name) {
            if (name[0] != '\0' && strlen(name) + 1 < sizeof(name)) {
                strncat(name, " ", sizeof(name) - strlen(name) - 1);
            }
            if (strlen(name) + strlen(token) < sizeof(name)) {
                strncat(name, token, sizeof(name) - strlen(name) - 1);
            }
        } else if (reading_value) {
            if (value[0] != '\0' && strlen(value) + 1 < sizeof(value)) {
                strncat(value, " ", sizeof(value) - strlen(value) - 1);
            }
            if (strlen(value) + strlen(token) < sizeof(value)) {
                strncat(value, token, sizeof(value) - strlen(value) - 1);
            }
        }
    }

    if (equals_ignore_case(name, "Hash")) {
        if (value[0] == '\0') {
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

    if (equals_ignore_case(name, "Clear Hash")) {
        engine_clear_hash();
        printf("info string Hash cleared\n");
        fflush(stdout);
        return;
    }

    printf("info string Unsupported option: %s\n", name[0] ? name : "(none)");
    fflush(stdout);
}

int safe_line_read(char* line_input)
{
    if (fgets(line_input, 8192, stdin) == NULL) {
        return 0;
    }
    size_t ender_pos = strcspn(line_input, "\r\n");
    if (ender_pos < strlen(line_input)) {
        line_input[ender_pos] = '\0'; // Remove newline characters
    }
    return 1;
}

void skip_whitespace(const char** str)
{
    while (**str && isspace((unsigned char)**str)) {
        (*str)++;
    }
}

void handle_go_command(const char* command)
{
    command += 2; // Skip "go"
    SearchLimits go_cmd         = { 0 };
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
                    ? move_from_uci_string(&current_board, moveToken, error_buf,
                          sizeof(error_buf))
                    : MOVE_NONE;

                if (move != MOVE_NONE) {
                    if (go_cmd.search_moves.count < 256) {
                        go_cmd.search_moves.moves[go_cmd.search_moves.count++]
                            = move;
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
        printf("  moves_until_next_time_control: %d\n",
               go_cmd.moves_until_next_time_control);
        printf("  depth_limit: %d\n", go_cmd.depth_limit);
        printf("  node_limit: %d\n", go_cmd.node_limit);
        printf(
            "  search_for_mate_in_n_moves: %d\n", go_cmd.search_for_mate_in_n_moves);
        printf("  time_limit_ms: %d\n", go_cmd.time_limit_ms);
        printf("  search_moves count: %d\n", go_cmd.search_moves.count);
    }

    char error_buf[128] = "";
    if (!engine_start_search(&go_cmd, error_buf, sizeof(error_buf))) {
        printf(
            "info string %s\n", error_buf[0] ? error_buf : "failed to start search");
        fflush(stdout);
    }
}

static void handle_perft_command(const char* command)
{
    engine_stop_search();
    command += 5; // Skip "perft"

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

        printf("perft depth %d nodes %" PRIu64 " nps %.0f time %.3f\n", depth, nodes,
               nps, elapsed);
        fflush(stdout);
    }
}
void handle_position_command(const char* command)
{
    engine_stop_search();

    command += 8; // Skip "position"
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
        size_t fen_length
            = moves_pos ? (size_t)(moves_pos - command) : strlen(command);
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
            if (!engine_apply_uci_move(
                    algebraic_move_str, error_buf, sizeof(error_buf))) {
                printf("info string error: %s, making move %s\n", error_buf,
                       algebraic_move_str);
                fflush(stdout);
            }
            command += move_length;
            skip_whitespace(&command);
            move_length = strcspn(command, " \r\n");
        }
    }
}
void uci_loop(void)
{
    static char line_input[8192]; // Buffer for reading input lines (max UCI
                                  // command length is 512 characters)
    while (safe_line_read(line_input)) {
        const char* p = line_input;
        skip_whitespace(&p);

        if (*p == '\0') {
            continue; // blank line
        }
        if (!strncmp(p, "quit", 4)
            && (p[4] == '\0' || isspace((unsigned char)p[4]))) {
            engine_shutdown();
            break;
        }

        else if (!strncmp(p, "isready", 7)
                 && (p[7] == '\0' || isspace((unsigned char)p[7]))) {
            printf("readyok\n");
            fflush(stdout);
        } else if (!strncmp(p, "uci", 3)
                   && (p[3] == '\0' || isspace((unsigned char)p[3]))) {
            engine_init();

            printf("id name Prophet dev\n"); // TODO: figure out how
                                             // to put actual version
                                             // info here
            printf("id author Nicolas Carbone\n");
            printf("option name Hash type spin default 64 min 1 max "
                   "1024\n");
            printf("option name Clear Hash type button\n");
            printf("uciok\n");
            fflush(stdout);
        } else if (!strncmp(p, "setoption", 9)
                   && (p[9] == '\0' || isspace((unsigned char)p[9]))) {
            handle_set_option_command(p);
        } else if (!strncmp(p, "ucinewgame", 10)
                   && (p[10] == '\0' || isspace((unsigned char)p[10]))) {
            engine_new_game();
        } else if (!strncmp(p, "position", 8)
                   && (p[8] == '\0' || isspace((unsigned char)p[8]))) {
            handle_position_command(p);
        } else if (!strncmp(p, "go", 2)
                   && (p[2] == '\0' || isspace((unsigned char)p[2]))) {
            handle_go_command(p);
        } else if (!strncmp(p, "perft", 5)
                   && (p[5] == '\0' || isspace((unsigned char)p[5]))) {
            handle_perft_command(p);
        } else if (!strncmp(p, "stop", 4)
                   && (p[4] == '\0' || isspace((unsigned char)p[4]))) {
            engine_stop_search();
        } else if (!strncmp(p, "ponderhit", 9)
                   && (p[9] == '\0' || isspace((unsigned char)p[9]))) {
            engine_handle_ponder_hit();
            if (engine_is_debug_mode()) {
                printf("info string ponderhit received\n");
            }
        } else if (!strncmp(p, "debug", 5)
                   && (p[5] == '\0' || isspace((unsigned char)p[5]))) {
            p += 5;
            skip_whitespace(&p);
            if (!strncmp(p, "on", 2)
                && (p[2] == '\0' || isspace((unsigned char)p[2]))) {
                engine_set_debug_mode(true);
                printf("info string Debug mode enabled\n");
            } else if (!strncmp(p, "off", 3)
                       && (p[3] == '\0' || isspace((unsigned char)p[3]))) {
                engine_set_debug_mode(false);
                printf("info string Debug mode disabled\n");
            } else {
                printf("info string Invalid debug command: %s\n", p);
            }
        } else if (!strncmp(p, "printboard", 10)
                   && (p[10] == '\0' || isspace((unsigned char)p[10]))) {
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
                printf("info string Ignoring unknown command: %s\n", p);
                fflush(stdout);
            }
        }
    }
}
