#include "chess/board/cboard.h"
#include "chess/board/zobrist.h"
#include "chess/movegen/move.h"
#include "chess/movegen/move_make.h"
#include "chess/movegen/movegen.h"
#include "chess/movegen/sliding_attacks.h"
#include "engine/search/search.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE_LINE_MAX 2048

typedef struct {
    int ply;
    char side;
    int score;
    double result;
    int completed_depth;
    int root_line_count;
    char fen[256];
} SampleTrace;

typedef struct {
    size_t game_index;
    size_t opening_index;
    size_t declared_move_count;
    size_t declared_sample_count;
    size_t move_count;
    size_t sample_count;
    char result[16];
    char terminal[16];
    char opening_fen[256];
    char** moves;
    SampleTrace* samples;
} GameTrace;

static int read_line(FILE* file, char line[TRACE_LINE_MAX])
{
    if (!fgets(line, TRACE_LINE_MAX, file)) {
        return 0;
    }
    size_t length = strlen(line);
    if (length > 0 && line[length - 1] == '\n') {
        line[--length] = '\0';
    } else if (!feof(file)) {
        return -1;
    }
    if (length > 0 && line[length - 1] == '\r') {
        line[length - 1] = '\0';
    }
    return 1;
}

static char* duplicate_string(const char* text)
{
    size_t length = strlen(text) + 1;
    char* copy = malloc(length);
    if (copy) {
        memcpy(copy, text, length);
    }
    return copy;
}

static bool parse_int(const char* text, int* value)
{
    char* end = NULL;
    errno = 0;
    long parsed = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' || parsed < INT_MIN
        || parsed > INT_MAX) {
        return false;
    }
    *value = (int)parsed;
    return true;
}

static bool parse_result(const char* text, double* result)
{
    if (!strcmp(text, "0.0")) {
        *result = 0.0;
        return true;
    }
    if (!strcmp(text, "0.5")) {
        *result = 0.5;
        return true;
    }
    if (!strcmp(text, "1.0")) {
        *result = 1.0;
        return true;
    }
    return false;
}

static void free_game(GameTrace* game)
{
    if (!game) {
        return;
    }
    for (size_t i = 0; i < game->move_count; i++) {
        free(game->moves[i]);
    }
    free(game->moves);
    free(game->samples);
    *game = (GameTrace) { 0 };
}

static bool parse_moves(const char* line, GameTrace* game)
{
    if (strncmp(line, "moves", 5) || (line[5] != '\0' && line[5] != ' ')) {
        return false;
    }

    char copy[TRACE_LINE_MAX];
    snprintf(copy, sizeof(copy), "%s", line + 5);
    char* token = strtok(copy, " ");
    while (token) {
        size_t count = game->move_count;
        char** moves = realloc(game->moves, (count + 1) * sizeof(*moves));
        if (!moves) {
            return false;
        }
        game->moves = moves;
        game->moves[count] = duplicate_string(token);
        if (!game->moves[count]) {
            return false;
        }
        game->move_count++;
        token = strtok(NULL, " ");
    }
    return true;
}

static bool parse_sample(const char* line, SampleTrace* sample)
{
    char copy[TRACE_LINE_MAX];
    snprintf(copy, sizeof(copy), "%s", line);
    char* tokens[16] = { 0 };
    size_t count = 0;
    char* token = strtok(copy, " ");
    while (token && count < sizeof(tokens) / sizeof(tokens[0])) {
        tokens[count++] = token;
        token = strtok(NULL, " ");
    }
    if (token || count != 13 || strcmp(tokens[0], "sample") || strlen(tokens[2]) != 1
        || (tokens[2][0] != 'w' && tokens[2][0] != 'b')) {
        return false;
    }

    if (!parse_int(tokens[1], &sample->ply) || !parse_int(tokens[3], &sample->score)
        || !parse_result(tokens[4], &sample->result)
        || !parse_int(tokens[5], &sample->completed_depth)
        || !parse_int(tokens[6], &sample->root_line_count)) {
        return false;
    }
    sample->side = tokens[2][0];
    int written = snprintf(sample->fen, sizeof(sample->fen), "%s %s %s %s %s %s", tokens[7],
                           tokens[8], tokens[9], tokens[10], tokens[11], tokens[12]);
    return written > 0 && (size_t)written < sizeof(sample->fen);
}

static bool parse_game_header(const char* line, GameTrace* game)
{
    char extra;
    int matched = sscanf(line, "game %zu opening %zu result %15s terminal %15s moves %zu samples %zu %c",
                         &game->game_index, &game->opening_index, game->result, game->terminal,
                         &game->declared_move_count, &game->declared_sample_count, &extra);
    if (matched != 6 || (strcmp(game->result, "draw") && strcmp(game->result, "white")
                         && strcmp(game->result, "black"))) {
        return false;
    }
    return !strcmp(game->terminal, "checkmate") || !strcmp(game->terminal, "stalemate")
        || !strcmp(game->terminal, "repetition") || !strcmp(game->terminal, "move_rule");
}

static bool parse_rejection(const char* line)
{
    char name[32];
    unsigned long long count;
    char extra;
    int matched = sscanf(line, "reject %31s %llu %c", name, &count, &extra);
    if (matched != 2 || count == 0) {
        return false;
    }
    return !strcmp(name, "terminal") || !strcmp(name, "in_check")
        || !strcmp(name, "mate_score") || !strcmp(name, "partial_search");
}

static const char* computed_terminal(const CBoard* board, const uint64_t* keys, size_t key_count)
{
    MoveList legal_moves;
    init_move_list(&legal_moves);
    generate_legal_moves((CBoard*)board, &legal_moves);
    if (legal_moves.count == 0) {
        return is_king_in_check((CBoard*)board, board->side_to_move) ? "checkmate" : "stalemate";
    }

    size_t repeats = 0;
    for (size_t i = 0; i < key_count; i++) {
        if (keys[i] == board->zobrist_key) {
            repeats++;
        }
    }
    if (repeats >= 3) {
        return "repetition";
    }
    if (board->half_move_clock >= 100) {
        return "move_rule";
    }
    return NULL;
}

static bool result_matches(const GameTrace* game, const char* terminal, const CBoard* board)
{
    const char* expected = "draw";
    if (!strcmp(terminal, "checkmate")) {
        expected = board->side_to_move == WHITE ? "black" : "white";
    }
    return !strcmp(game->result, expected);
}

static bool validate_game(GameTrace* game, const char* path)
{
    CBoard board;
    if (!fen_string_to_cboard(game->opening_fen, &board)) {
        fprintf(stderr, "%s: game %zu has an invalid opening FEN\n", path, game->game_index);
        return false;
    }

    uint64_t* keys = calloc(game->move_count + 1, sizeof(*keys));
    if (!keys) {
        return false;
    }
    keys[0] = board.zobrist_key;
    size_t sample_index = 0;
    bool valid = true;

    for (size_t ply = 0; ply <= game->move_count; ply++) {
        const char* terminal = computed_terminal(&board, keys, ply + 1);
        while (sample_index < game->sample_count
               && game->samples[sample_index].ply == (int)ply) {
            SampleTrace* sample = &game->samples[sample_index];
            if (terminal || is_king_in_check(&board, board.side_to_move)
                || board.half_move_clock >= 100 || sample->completed_depth <= 0
                || sample->root_line_count <= 0 || sample->score >= MATE_THRESHOLD
                || sample->score <= -MATE_THRESHOLD
                || (sample->side == 'w') != (board.side_to_move == WHITE)) {
                fprintf(stderr, "%s: game %zu sample at ply %d violates a V1 filter\n", path,
                        game->game_index, sample->ply);
                valid = false;
            }

            CBoard sample_board;
            if (!fen_string_to_cboard(sample->fen, &sample_board)) {
                fprintf(stderr, "%s: game %zu sample at ply %d has invalid FEN\n", path,
                        game->game_index, sample->ply);
                valid = false;
            } else {
                char* expected_fen = cboard_to_fen(&board);
                char* actual_fen = cboard_to_fen(&sample_board);
                if (!expected_fen || !actual_fen || strcmp(expected_fen, actual_fen)) {
                    fprintf(stderr, "%s: game %zu sample at ply %d does not match replay state\n",
                            path, game->game_index, sample->ply);
                    valid = false;
                }
                free(expected_fen);
                free(actual_fen);
            }

            double expected_result = 0.5;
            if (!strcmp(game->result, "white")) {
                expected_result = sample->side == 'w' ? 1.0 : 0.0;
            } else if (!strcmp(game->result, "black")) {
                expected_result = sample->side == 'b' ? 1.0 : 0.0;
            }
            if (fabs(sample->result - expected_result) > 0.00001) {
                fprintf(stderr, "%s: game %zu sample at ply %d has the wrong result\n", path,
                        game->game_index, sample->ply);
                valid = false;
            }
            sample_index++;
        }
        if (sample_index < game->sample_count && game->samples[sample_index].ply < (int)ply) {
            fprintf(stderr, "%s: game %zu samples are not ordered by ply\n", path,
                    game->game_index);
            valid = false;
            break;
        }

        if (ply == game->move_count) {
            if (!terminal || strcmp(terminal, game->terminal) || !result_matches(game, terminal, &board)) {
                fprintf(stderr, "%s: game %zu has an incorrect final result or terminal reason\n",
                        path, game->game_index);
                valid = false;
            }
            break;
        }
        if (terminal) {
            fprintf(stderr, "%s: game %zu continues after %s at ply %zu\n", path,
                    game->game_index, terminal, ply);
            valid = false;
            break;
        }

        char error[128] = "";
        Move move = move_from_uci_string(&board, game->moves[ply], error, sizeof(error));
        if (move == MOVE_NONE) {
            fprintf(stderr, "%s: game %zu move %zu is illegal: %s\n", path, game->game_index,
                    ply, error);
            valid = false;
            break;
        }
        make_move(&board, move);
        keys[ply + 1] = board.zobrist_key;
    }

    if (sample_index != game->sample_count) {
        fprintf(stderr, "%s: game %zu contains a sample outside its move range\n", path,
                game->game_index);
        valid = false;
    }
    free(keys);
    return valid;
}

static bool validate_file(const char* path, size_t* games, size_t* samples)
{
    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "%s: cannot open trace\n", path);
        return false;
    }

    char line[TRACE_LINE_MAX];
    bool valid = true;
    if (read_line(file, line) != 1 || strcmp(line, "prophet-datagen-v1")) {
        valid = false;
    }
    if (valid && (read_line(file, line) != 1 || strncmp(line, "worker ", 7) || !line[7])) {
        valid = false;
    }
    if (valid && (read_line(file, line) != 1 || strncmp(line, "worker_seed ", 12) || !line[12])) {
        valid = false;
    }
    if (valid && (read_line(file, line) != 1 || strncmp(line, "openings_sha256 ", 16)
                  || strlen(line + 16) != 64)) {
        valid = false;
    } else if (valid) {
        for (size_t i = 0; i < 64; i++) {
            if (!isxdigit((unsigned char)line[16 + i])) {
                valid = false;
                break;
            }
        }
    }
    if (valid && (read_line(file, line) != 1 || line[0])) {
        valid = false;
    }
    if (!valid) {
        fprintf(stderr, "%s: invalid trace header\n", path);
        fclose(file);
        return false;
    }

    int line_status = read_line(file, line);
    while (line_status == 1) {
        if (!line[0]) {
            continue;
        }
        GameTrace game = { 0 };
        if (!parse_game_header(line, &game)) {
            fprintf(stderr, "%s: unexpected trace line: %s\n", path, line);
            valid = false;
            break;
        }
        if (read_line(file, line) != 1 || strncmp(line, "opening_fen ", 12)
            || !line[12] || snprintf(game.opening_fen, sizeof(game.opening_fen), "%s", line + 12)
                >= (int)sizeof(game.opening_fen)) {
            valid = false;
        }
        if (valid && (read_line(file, line) != 1 || !parse_moves(line, &game))) {
            valid = false;
        }
        if (valid && game.move_count != game.declared_move_count) {
            valid = false;
        }
        while (valid && (line_status = read_line(file, line)) == 1 && strcmp(line, "endgame")) {
            if (!strncmp(line, "sample ", 7)) {
                SampleTrace sample;
                if (!parse_sample(line, &sample)
                    || sample.ply < 0
                    || (game.sample_count > 0 && sample.ply <= game.samples[game.sample_count - 1].ply)) {
                    valid = false;
                    break;
                }
                SampleTrace* updated_samples
                    = realloc(game.samples, (game.sample_count + 1) * sizeof(*updated_samples));
                if (!updated_samples) {
                    valid = false;
                    break;
                }
                game.samples = updated_samples;
                game.samples[game.sample_count++] = sample;
            } else if (!strncmp(line, "reject ", 7)) {
                if (!parse_rejection(line)) {
                    valid = false;
                    break;
                }
            } else {
                valid = false;
                break;
            }
        }
        if (valid && (!line[0] || strcmp(line, "endgame")
                      || game.sample_count != game.declared_sample_count)) {
            valid = false;
        }
        line_status = read_line(file, line);
        if (valid && line_status < 0) {
            fprintf(stderr, "%s: overlong trace line\n", path);
            valid = false;
        } else if (valid && line_status == 1 && line[0]) {
            fprintf(stderr, "%s: missing trace block separator\n", path);
            valid = false;
        }
        if (valid && !validate_game(&game, path)) {
            valid = false;
        }
        if (valid) {
            (*games)++;
            *samples += game.sample_count;
        }
        free_game(&game);
        if (!valid) {
            break;
        }
        line_status = read_line(file, line);
    }
    if (line_status < 0 || ferror(file)) {
        valid = false;
    }
    fclose(file);
    return valid;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: datagen_trace_test <trace>...\n");
        return 2;
    }
    init_sliding_attacks();
    init_zobrist_keys();

    size_t games = 0;
    size_t samples = 0;
    for (int i = 1; i < argc; i++) {
        if (!validate_file(argv[i], &games, &samples)) {
            return 1;
        }
    }
    printf("Datagen trace audit passed: %zu games, %zu samples\n", games, samples);
    return 0;
}
