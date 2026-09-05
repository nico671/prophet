#define _POSIX_C_SOURCE 200809L

#include "engine/datagen/datagen.h"

#include "chess/board/cboard.h"
#include "chess/board/zobrist.h"
#include "chess/core/bitboard.h"
#include "chess/movegen/move.h"
#include "chess/movegen/move_make.h"
#include "chess/movegen/movegen.h"
#include "chess/movegen/sliding_attacks.h"
#include "chess/utils/prng.h"
#include "chess/utils/sha256.h"
#include "engine/datagen/binpack_writer.h"
#include "engine/eval/hceval.h"
#include "engine/search/search.h"
#include "engine/tt/tt.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define DATAGEN_PATH_MAX 1024
#define DATAGEN_LINE_MAX 2048
#define DATAGEN_CONFIG_VERSION 1
#define DATAGEN_MAX_WORKERS 256
#define DATAGEN_MAX_HASH_MB 1024
#define DATAGEN_MAX_GAME_PLY 100000

#ifndef PROPHET_GIT_COMMIT
#define PROPHET_GIT_COMMIT "unknown"
#endif

typedef enum {
    GAME_DRAW,
    GAME_WHITE_WIN,
    GAME_BLACK_WIN,
} GameResult;

typedef enum {
    COLOR_PRESERVE,
    COLOR_SWAP,
    COLOR_ALTERNATE,
} ColorAssignment;

typedef enum {
    TERMINAL_CHECKMATE,
    TERMINAL_STALEMATE,
    TERMINAL_REPETITION,
    TERMINAL_MOVE_RULE,
    TERMINAL_COUNT,
} TerminalReason;

typedef enum {
    REJECT_TERMINAL,
    REJECT_IN_CHECK,
    REJECT_MATE_SCORE,
    REJECT_PARTIAL_SEARCH,
    REJECT_COUNT,
} RejectionReason;

typedef struct {
    char openings_path[DATAGEN_PATH_MAX];
    char openings_sha256[65];
    size_t opening_first;
    size_t opening_count;
    size_t games;
    int workers;
    uint64_t root_seed;
    uint64_t worker_seed_base;
    int nodes_per_move;
    int hash_mb;
    bool clear_hash_per_game;
    int early_ply_limit;
    int early_multipv;
    int sample_start_ply;
    int sample_interval;
    uint64_t sample_offset_seed;
    int max_game_ply;
    size_t shard_game_limit;
    ColorAssignment color_assignment;
} DatagenConfig;

typedef struct {
    CBoard* boards;
    size_t count;
    size_t capacity;
} OpeningSet;

typedef struct {
    CBoard board;
    int ply;
    int score;
    int completed_depth;
    int root_line_count;
    Move best_move;
    Color side_to_move;
    double result;
} DatagenSample;

typedef struct {
    size_t game_index;
    size_t opening_index;
    CBoard opening_board;
    Move* moves;
    size_t move_count;
    DatagenSample* samples;
    size_t sample_count;
    GameResult result;
    TerminalReason terminal_reason;
    uint64_t rejections[REJECT_COUNT];
} DatagenGame;

typedef struct {
    FILE* file;
    BinpackWriter binpack;
    char output_base[DATAGEN_PATH_MAX];
    char binpack_partial[DATAGEN_PATH_MAX];
    char binpack_final[DATAGEN_PATH_MAX];
    char openings_sha256[65];
    int worker_id;
    uint64_t worker_seed;
    size_t shard_game_limit;
    size_t shard_index;
    size_t games_in_shard;
    size_t records_in_shard;
    uint64_t rejections[REJECT_COUNT];
    size_t results[3];
    int score_min;
    int score_max;
    int ply_min;
    int ply_max;
    const DatagenConfig* config;
} TraceWriter;

static void print_usage(FILE* stream)
{
    fprintf(stream, "Usage: prophet datagen --config <path> --output <prefix>\n");
}

/**
 * @brief a static utility that removes leading and trailing whitespace characters from a given
 * string.
 * @note Modifies the string in place
 * @param text String to manipulate
 * @return a pointer to the first non-whitespace character.
 */
static char* trim(char* text)
{
    while (*text && isspace((unsigned char)*text)) {
        text++;
    }
    char* end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return text;
}

static bool copy_value_string(const char* value, char* output, size_t output_size)
{
    size_t length = strlen(value);
    if (length >= 2 && value[0] == '"' && value[length - 1] == '"') {
        value++;
        length -= 2;
    } else if (length == 0 || strchr(value, '"') != NULL) {
        return false;
    }
    if (length >= output_size) {
        return false;
    }
    memcpy(output, value, length);
    output[length] = '\0';
    return true;
}

static bool parse_unsigned(const char* value, uint64_t* output)
{
    if (*value == '-') {
        return false;
    }
    char* end                 = NULL;
    errno                     = 0;
    unsigned long long parsed = strtoull(value, &end, 0);
    if (errno == ERANGE || end == value || *trim(end) != '\0') {
        return false;
    }
    *output = (uint64_t)parsed;
    return true;
}

static bool parse_positive_int(const char* value, int* output)
{
    uint64_t parsed;
    if (!parse_unsigned(value, &parsed) || parsed == 0 || parsed > INT_MAX) {
        return false;
    }
    *output = (int)parsed;
    return true;
}

static bool parse_positive_size(const char* value, size_t* output)
{
    uint64_t parsed;
    if (!parse_unsigned(value, &parsed) || parsed == 0 || (uint64_t)(size_t)parsed != parsed) {
        return false;
    }
    *output = (size_t)parsed;
    return true;
}

static bool parse_bool(const char* value, bool* output)
{
    if (!strcmp(value, "true")) {
        *output = true;
        return true;
    }
    if (!strcmp(value, "false")) {
        *output = false;
        return true;
    }
    return false;
}

static bool parse_color_assignment(const char* value, ColorAssignment* output)
{
    if (!strcmp(value, "preserve")) {
        *output = COLOR_PRESERVE;
    } else if (!strcmp(value, "swap")) {
        *output = COLOR_SWAP;
    } else if (!strcmp(value, "alternate")) {
        *output = COLOR_ALTERNATE;
    } else {
        return false;
    }
    return true;
}

static bool parse_config(const char* path, DatagenConfig* config, char* error, size_t error_size)
{
    if (!path || !config) {
        return false;
    }

    *config    = (DatagenConfig) { 0 };
    FILE* file = fopen(path, "r");
    if (!file) {
        snprintf(error, error_size, "cannot open config: %s", path);
        return false;
    }

    enum {
        HAVE_VERSION         = 1ULL << 0,
        HAVE_OPENINGS        = 1ULL << 1,
        HAVE_OPENINGS_HASH   = 1ULL << 2,
        HAVE_OPENING_FIRST   = 1ULL << 3,
        HAVE_OPENING_COUNT   = 1ULL << 4,
        HAVE_GAMES           = 1ULL << 5,
        HAVE_WORKERS         = 1ULL << 6,
        HAVE_ROOT_SEED       = 1ULL << 7,
        HAVE_WORKER_SEED     = 1ULL << 8,
        HAVE_NODES           = 1ULL << 9,
        HAVE_HASH_MB         = 1ULL << 10,
        HAVE_CLEAR_HASH      = 1ULL << 11,
        HAVE_EARLY_LIMIT     = 1ULL << 12,
        HAVE_EARLY_MULTIPV   = 1ULL << 13,
        HAVE_SAMPLE_START    = 1ULL << 14,
        HAVE_SAMPLE_INTERVAL = 1ULL << 15,
        HAVE_SAMPLE_OFFSET   = 1ULL << 16,
        HAVE_MAX_PLY         = 1ULL << 17,
        HAVE_SHARD_LIMIT     = 1ULL << 18,
        HAVE_COLOR           = 1ULL << 19,
    };
    // number of fields that we need to see for success (20)
    const uint64_t required = (1ULL << 20) - 1;
    uint64_t seen           = 0;
    char line[DATAGEN_LINE_MAX];
    size_t line_number = 0;
    bool success       = true;

    while (fgets(line, sizeof(line), file)) {
        line_number++;
        if (!strchr(line, '\n') && !feof(file)) {
            snprintf(error, error_size, "config line %zu is too long", line_number);
            success = false;
            break;
        }
        char* text = trim(line);
        if (*text == '\0' || *text == '#') {
            continue;
        }
        char* equals = strchr(text, '=');
        if (!equals) {
            snprintf(error, error_size, "config line %zu must contain '='", line_number);
            success = false;
            break;
        }
        *equals     = '\0';
        char* key   = trim(text);
        char* value = trim(equals + 1);
        if (*key == '\0' || *value == '\0') {
            snprintf(error, error_size, "config line %zu has an empty key or value", line_number);
            success = false;
            break;
        }

        uint64_t bit = 0;
        bool valid   = true;
        if (!strcmp(key, "version")) {
            uint64_t parsed;
            bit   = HAVE_VERSION;
            valid = parse_unsigned(value, &parsed) && parsed == DATAGEN_CONFIG_VERSION;
        } else if (!strcmp(key, "openings")) {
            bit   = HAVE_OPENINGS;
            valid = copy_value_string(value, config->openings_path, sizeof(config->openings_path));
        } else if (!strcmp(key, "openings_sha256")) {
            bit   = HAVE_OPENINGS_HASH;
            valid = copy_value_string(value, config->openings_sha256,
                                      sizeof(config->openings_sha256));
            if (valid && strlen(config->openings_sha256) != 64) {
                valid = false;
            }
        } else if (!strcmp(key, "opening_first")) {
            bit = HAVE_OPENING_FIRST;
            uint64_t parsed;
            valid = parse_unsigned(value, &parsed) && (size_t)parsed == parsed;
            if (valid) {
                config->opening_first = (size_t)parsed;
            }
        } else if (!strcmp(key, "opening_count")) {
            bit   = HAVE_OPENING_COUNT;
            valid = parse_positive_size(value, &config->opening_count);
        } else if (!strcmp(key, "games")) {
            bit   = HAVE_GAMES;
            valid = parse_positive_size(value, &config->games);
        } else if (!strcmp(key, "workers")) {
            bit   = HAVE_WORKERS;
            valid = parse_positive_int(value, &config->workers)
                && config->workers <= DATAGEN_MAX_WORKERS;
        } else if (!strcmp(key, "root_seed")) {
            bit   = HAVE_ROOT_SEED;
            valid = parse_unsigned(value, &config->root_seed);
        } else if (!strcmp(key, "worker_seed_base")) {
            bit   = HAVE_WORKER_SEED;
            valid = parse_unsigned(value, &config->worker_seed_base);
        } else if (!strcmp(key, "nodes_per_move")) {
            bit   = HAVE_NODES;
            valid = parse_positive_int(value, &config->nodes_per_move);
        } else if (!strcmp(key, "hash_mb")) {
            bit   = HAVE_HASH_MB;
            valid = parse_positive_int(value, &config->hash_mb)
                && config->hash_mb <= DATAGEN_MAX_HASH_MB;
        } else if (!strcmp(key, "clear_hash_per_game")) {
            bit   = HAVE_CLEAR_HASH;
            valid = parse_bool(value, &config->clear_hash_per_game);
        } else if (!strcmp(key, "early_ply_limit")) {
            bit   = HAVE_EARLY_LIMIT;
            valid = parse_positive_int(value, &config->early_ply_limit);
        } else if (!strcmp(key, "early_multipv")) {
            bit   = HAVE_EARLY_MULTIPV;
            valid = parse_positive_int(value, &config->early_multipv)
                && config->early_multipv <= MAX_LEGAL_MOVES;
        } else if (!strcmp(key, "sample_start_ply")) {
            bit = HAVE_SAMPLE_START;
            uint64_t parsed;
            valid = parse_unsigned(value, &parsed) && parsed <= INT_MAX;
            if (valid) {
                config->sample_start_ply = (int)parsed;
            }
        } else if (!strcmp(key, "sample_interval")) {
            bit   = HAVE_SAMPLE_INTERVAL;
            valid = parse_positive_int(value, &config->sample_interval);
        } else if (!strcmp(key, "sample_offset_seed")) {
            bit   = HAVE_SAMPLE_OFFSET;
            valid = parse_unsigned(value, &config->sample_offset_seed);
        } else if (!strcmp(key, "max_game_ply")) {
            bit   = HAVE_MAX_PLY;
            valid = parse_positive_int(value, &config->max_game_ply);
            if (valid
                && (config->max_game_ply > DATAGEN_MAX_GAME_PLY || config->max_game_ply > 0x3fff)) {
                valid = false;
            }
        } else if (!strcmp(key, "shard_game_limit")) {
            bit   = HAVE_SHARD_LIMIT;
            valid = parse_positive_size(value, &config->shard_game_limit);
        } else if (!strcmp(key, "color_assignment")) {
            char assignment[32];
            bit   = HAVE_COLOR;
            valid = copy_value_string(value, assignment, sizeof(assignment))
                && parse_color_assignment(assignment, &config->color_assignment);
        } else {
            snprintf(error, error_size, "unknown config key on line %zu: %s", line_number, key);
            success = false;
            break;
        }

        if (!valid) {
            snprintf(error, error_size, "invalid value for %s on line %zu", key, line_number);
            success = false;
            break;
        }
        if (seen & bit) {
            snprintf(error, error_size, "duplicate config key on line %zu: %s", line_number, key);
            success = false;
            break;
        }
        seen |= bit;
    }

    if (ferror(file)) {
        snprintf(error, error_size, "cannot read config: %s", path);
        success = false;
    }
    fclose(file);
    if (!success) {
        return false;
    }
    if ((seen & required) != required) {
        snprintf(error, error_size, "config is missing one or more required fields");
        return false;
    }
    if (config->sample_start_ply < 0 || config->sample_start_ply > config->max_game_ply
        || config->early_ply_limit > config->max_game_ply) {
        snprintf(error, error_size, "config ply limits are inconsistent");
        return false;
    }
    for (size_t i = 0; i < 64; i++) {
        if (!isxdigit((unsigned char)config->openings_sha256[i])) {
            snprintf(error, error_size, "openings_sha256 must contain hexadecimal digits");
            return false;
        }
        config->openings_sha256[i] = (char)tolower((unsigned char)config->openings_sha256[i]);
    }
    return true;
}

static void free_openings(OpeningSet* openings)
{
    free(openings->boards);
    *openings = (OpeningSet) { 0 };
}

static bool load_openings(const DatagenConfig* config, OpeningSet* openings, char* error,
                          size_t error_size)
{
    char actual_hash[65];
    if (!sha256_file(config->openings_path, actual_hash)) {
        snprintf(error, error_size, "cannot hash opening file: %s", config->openings_path);
        return false;
    }
    if (strcasecmp(actual_hash, config->openings_sha256) != 0) {
        snprintf(error, error_size, "opening file SHA-256 does not match configuration");
        return false;
    }

    FILE* file = fopen(config->openings_path, "r");
    if (!file) {
        snprintf(error, error_size, "cannot open opening file: %s", config->openings_path);
        return false;
    }
    *openings = (OpeningSet) { 0 };
    char line[DATAGEN_LINE_MAX];
    size_t line_number = 0;
    bool success       = true;
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        if (!strchr(line, '\n') && !feof(file)) {
            snprintf(error, error_size, "opening line %zu is too long", line_number);
            success = false;
            break;
        }
        char* text = trim(line);
        if (*text == '\0' || *text == '#') {
            continue;
        }
        CBoard board;
        if (!fen_string_to_cboard(text, &board)) {
            snprintf(error, error_size, "invalid opening FEN on line %zu", line_number);
            success = false;
            break;
        }
        if (openings->count == openings->capacity) {
            size_t capacity = openings->capacity == 0 ? 1024 : openings->capacity * 2;
            CBoard* boards  = realloc(openings->boards, capacity * sizeof(*boards));
            if (!boards) {
                snprintf(error, error_size, "out of memory while loading openings");
                success = false;
                break;
            }
            openings->boards   = boards;
            openings->capacity = capacity;
        }
        openings->boards[openings->count++] = board;
    }
    if (ferror(file)) {
        snprintf(error, error_size, "cannot read opening file");
        success = false;
    }
    fclose(file);
    if (!success) {
        free_openings(openings);
        return false;
    }
    if (config->opening_first >= openings->count
        || config->opening_count > openings->count - config->opening_first) {
        snprintf(error, error_size, "opening selection range is outside the opening file");
        free_openings(openings);
        return false;
    }
    return true;
}

static uint64_t derived_seed(uint64_t base, uint64_t label)
{
    ranctx random;
    raninit(&random, base ^ (label * 0x9e3779b97f4a7c15ULL));
    return ranval(&random);
}

static void swap_colors_and_ranks(const CBoard* source, CBoard* target)
{
    *target                  = (CBoard) { 0 };
    target->side_to_move     = color_opposite(source->side_to_move);
    target->half_move_clock  = source->half_move_clock;
    target->full_move_number = source->full_move_number;
    target->ep_square
        = source->ep_square == NO_SQUARE ? NO_SQUARE : (uint8_t)(source->ep_square ^ 56);

    if (U8_CHECK_BIT(source->castling_rights, 3)) {
        U8_SET_BIT(target->castling_rights, 1);
    }
    if (U8_CHECK_BIT(source->castling_rights, 2)) {
        U8_SET_BIT(target->castling_rights, 0);
    }
    if (U8_CHECK_BIT(source->castling_rights, 1)) {
        U8_SET_BIT(target->castling_rights, 3);
    }
    if (U8_CHECK_BIT(source->castling_rights, 0)) {
        U8_SET_BIT(target->castling_rights, 2);
    }

    for (Color color = WHITE; color <= BLACK; color++) {
        for (PieceType piece = PAWN; piece <= KING; piece++) {
            Bitboard pieces = source->piece_bbs[color][piece];
            while (pieces) {
                Square square       = (Square)bitboard_pop_lsb_unsafe(&pieces);
                Square transformed  = square_flip_vertical(square);
                Color swapped_color = color_opposite(color);
                bitboard_set_square_bit(&target->piece_bbs[swapped_color][piece], transformed);
                target->piece_at_square[transformed] = piece;
            }
        }
    }
    target->occupancy_bbs[WHITE] = 0;
    target->occupancy_bbs[BLACK] = 0;
    for (Color color = WHITE; color <= BLACK; color++) {
        for (PieceType piece = PAWN; piece <= KING; piece++) {
            target->occupancy_bbs[color] |= target->piece_bbs[color][piece];
        }
    }
    target->occupancy_bbs[2] = target->occupancy_bbs[WHITE] | target->occupancy_bbs[BLACK];
    compute_zobrist_key(target);
}

static bool position_has_repetition(const uint64_t* keys, size_t count, uint64_t key)
{
    size_t repeats = 0;
    for (size_t i = 0; i < count; i++) {
        if (keys[i] == key) {
            repeats++;
        }
    }
    return repeats >= 3;
}

static void add_position_history(PositionHistory* history, uint64_t key, uint16_t half_move_clock)
{
    if (half_move_clock == 0) {
        history->count   = 1;
        history->keys[0] = key;
        return;
    }
    if (history->count == MAX_POSITION_HISTORY) {
        memmove(history->keys, history->keys + 1,
                (MAX_POSITION_HISTORY - 1) * sizeof(history->keys[0]));
        history->count--;
    }
    history->keys[history->count++] = key;
}

static const char* result_name(GameResult result)
{
    return result == GAME_WHITE_WIN ? "white" : result == GAME_BLACK_WIN ? "black" : "draw";
}

static const char* terminal_name(TerminalReason reason)
{
    static const char* names[TERMINAL_COUNT] = {
        "checkmate",
        "stalemate",
        "repetition",
        "move_rule",
    };
    return names[reason];
}

static const char* rejection_name(RejectionReason reason)
{
    static const char* names[REJECT_COUNT] = {
        "terminal",
        "in_check",
        "mate_score",
        "partial_search",
    };
    return names[reason];
}

static double sample_result(GameResult result, Color side_to_move)
{
    if (result == GAME_DRAW) {
        return 0.5;
    }
    Color winner = result == GAME_WHITE_WIN ? WHITE : BLACK;
    return winner == side_to_move ? 1.0 : 0.0;
}

static bool escape_json(const char* text, char* output, size_t capacity)
{
    size_t used = 0;
    for (const unsigned char* cursor = (const unsigned char*)text; *cursor; cursor++) {
        size_t needed = *cursor < 0x20 ? 6 : (*cursor == '"' || *cursor == '\\') ? 2 : 1;
        if (used + needed >= capacity) {
            return false;
        }
        if (*cursor < 0x20) {
            snprintf(output + used, capacity - used, "\\u%04x", *cursor);
        } else {
            if (needed == 2) {
                output[used] = '\\';
            }
            output[used + needed - 1] = (char)*cursor;
        }
        used += needed;
    }
    output[used] = '\0';
    return true;
}

static bool trace_finish_shard(TraceWriter* writer)
{
    if (writer->file && fclose(writer->file) != 0) {
        writer->file = NULL;
        return false;
    }
    writer->file = NULL;
    if (!binpack_close(&writer->binpack)) {
        return false;
    }
    struct stat payload_stat;
    char payload_hash[65];
    if (stat(writer->binpack_partial, &payload_stat) != 0
        || !sha256_file(writer->binpack_partial, payload_hash)) {
        return false;
    }
    char normalized[2048];
    const DatagenConfig* config = writer->config;
    int normalized_length       = snprintf(
        normalized, sizeof(normalized),
        "version=%d\nopenings_sha256=%s\nopening_first=%zu\nopening_count=%zu\ngames=%zu\nworkers=%"
        "d\nroot_seed=%" PRIu64 "\nworker_seed_base=%" PRIu64
        "\nnodes_per_move=%d\nhash_mb=%d\nclear_hash_per_game=%s\nearly_ply_limit=%d\nearly_"
        "multipv=%d\nsample_start_ply=%d\nsample_interval=%d\nsample_offset_seed=%" PRIu64
        "\nmax_game_ply=%d\nshard_game_limit=%zu\ncolor_assignment=%d\n",
        DATAGEN_CONFIG_VERSION, config->openings_sha256, config->opening_first,
        config->opening_count, config->games, config->workers, config->root_seed,
        config->worker_seed_base, config->nodes_per_move, config->hash_mb,
        config->clear_hash_per_game ? "true" : "false", config->early_ply_limit,
        config->early_multipv, config->sample_start_ply, config->sample_interval,
        config->sample_offset_seed, config->max_game_ply, config->shard_game_limit,
        config->color_assignment);
    if (normalized_length < 0 || (size_t)normalized_length >= sizeof(normalized)) {
        return false;
    }
    char config_hash[65];
    if (!sha256_bytes(normalized, (size_t)normalized_length, config_hash)) {
        return false;
    }
    char normalized_json[4096];
    const char* basename = strrchr(writer->binpack_final, '/');
    basename             = basename ? basename + 1 : writer->binpack_final;
    char filename_json[6 * DATAGEN_PATH_MAX];
    if (!escape_json(normalized, normalized_json, sizeof(normalized_json))
        || !escape_json(basename, filename_json, sizeof(filename_json))) {
        return false;
    }
    char manifest_partial[DATAGEN_PATH_MAX];
    char manifest_final[DATAGEN_PATH_MAX];
    if (snprintf(manifest_partial, sizeof(manifest_partial), "%s.manifest.json.partial",
                 writer->binpack_final)
            >= (int)sizeof(manifest_partial)
        || snprintf(manifest_final, sizeof(manifest_final), "%s.manifest.json",
                    writer->binpack_final)
            >= (int)sizeof(manifest_final)) {
        return false;
    }
    FILE* manifest = fopen(manifest_partial, "wx");
    if (!manifest) {
        return false;
    }
    const char* color = config->color_assignment == COLOR_PRESERVE ? "preserve"
        : config->color_assignment == COLOR_SWAP                   ? "swap"
                                                                   : "alternate";
    int wrote         = fprintf(
        manifest,
        "{\n  \"schema_version\": 1,\n  \"data_file\": \"%s\",\n  \"byte_size\": %lld,\n  "
        "\"sha256\": \"%s\",\n  \"record_count\": %zu,\n  \"game_count\": %zu,\n  "
        "\"generator_commit\": \"%s\",\n  \"engine_commit\": \"%s\",\n  \"teacher\": \"HCE\",\n  "
        "\"openings_sha256\": \"%s\",\n  \"root_seed\": %" PRIu64 ",\n  \"worker_seed\": %" PRIu64
        ",\n  \"normalized_config\": \"%s\",\n  \"normalized_config_sha256\": \"%s\",\n  "
        "\"settings\": {\"nodes_per_move\": %d, \"early_ply_limit\": %d, \"early_multipv\": %d, "
        "\"sample_start_ply\": %d, \"sample_interval\": %d, \"max_game_ply\": %d, "
        "\"color_assignment\": \"%s\"},\n  \"results\": {\"white_wins\": %zu, \"draws\": %zu, "
        "\"black_wins\": %zu},\n  \"rejections\": {\"terminal\": %" PRIu64
        ", \"in_check\": %" PRIu64 ", \"mate_score\": %" PRIu64 ", \"partial_search\": %" PRIu64
        "},\n  \"score_summary\": {\"min\": %d, \"max\": %d},\n  \"ply_summary\": {\"min\": %d, "
        "\"max\": %d},\n  \"licenses\": {\"source\": \"GPL-3.0-only\", \"artifact\": "
        "\"project-controlled self-play\"}\n}\n",
        filename_json, (long long)payload_stat.st_size, payload_hash, writer->records_in_shard,
        writer->games_in_shard, PROPHET_GIT_COMMIT, PROPHET_GIT_COMMIT, writer->openings_sha256,
        config->root_seed, writer->worker_seed, normalized_json, config_hash,
        config->nodes_per_move, config->early_ply_limit, config->early_multipv,
        config->sample_start_ply, config->sample_interval, config->max_game_ply, color,
        writer->results[GAME_WHITE_WIN], writer->results[GAME_DRAW],
        writer->results[GAME_BLACK_WIN], writer->rejections[REJECT_TERMINAL],
        writer->rejections[REJECT_IN_CHECK], writer->rejections[REJECT_MATE_SCORE],
        writer->rejections[REJECT_PARTIAL_SEARCH], writer->records_in_shard ? writer->score_min : 0,
        writer->records_in_shard ? writer->score_max : 0,
        writer->records_in_shard ? writer->ply_min : 0,
        writer->records_in_shard ? writer->ply_max : 0);
    int close_status = fclose(manifest);
    bool success     = wrote >= 0 && close_status == 0;
    if (!success || rename(writer->binpack_partial, writer->binpack_final) != 0
        || rename(manifest_partial, manifest_final) != 0) {
        return false;
    }
    return true;
}

static bool trace_open(TraceWriter* writer)
{
    char path[DATAGEN_PATH_MAX];
    int written = snprintf(path, sizeof(path), "%s.worker-%04d.part-%04zu.games",
                           writer->output_base, writer->worker_id, writer->shard_index);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        fprintf(stderr, "datagen output path is too long\n");
        return false;
    }
    int binpack_written = snprintf(writer->binpack_final, sizeof(writer->binpack_final),
                                   "%s.worker-%04d.part-%04zu.binpack", writer->output_base,
                                   writer->worker_id, writer->shard_index);
    if (binpack_written < 0 || (size_t)binpack_written >= sizeof(writer->binpack_final)
        || snprintf(writer->binpack_partial, sizeof(writer->binpack_partial), "%s.partial",
                    writer->binpack_final)
            >= (int)sizeof(writer->binpack_partial)) {
        return false;
    }
    // Reject orphaned artifacts as well as complete earlier runs.
    const char* suffixes[] = { "", ".partial", ".manifest.json", ".manifest.json.partial" };
    for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
        char artifact[DATAGEN_PATH_MAX];
        if (snprintf(artifact, sizeof(artifact), "%s%s", writer->binpack_final, suffixes[i])
            >= (int)sizeof(artifact)) {
            return false;
        }
        struct stat existing;
        if (lstat(artifact, &existing) == 0 || errno != ENOENT) {
            fprintf(stderr, "datagen output already exists or cannot be checked: %s\n", artifact);
            return false;
        }
    }
    writer->file = fopen(path, "wx");
    if (!writer->file) {
        fprintf(stderr, "cannot open datagen output %s: %s\n", path, strerror(errno));
        return false;
    }
    fprintf(writer->file,
            "prophet-datagen-v1\nworker %d\nworker_seed %" PRIu64 "\nopenings_sha256 %s\n\n",
            writer->worker_id, writer->worker_seed, writer->openings_sha256);
    if (ferror(writer->file)) {
        fclose(writer->file);
        writer->file = NULL;
        return false;
    }
    if (!binpack_open(&writer->binpack, writer->binpack_partial)) {
        fclose(writer->file);
        writer->file = NULL;
        return false;
    }
    writer->records_in_shard = 0;
    writer->rejections[0] = writer->rejections[1] = writer->rejections[2] = writer->rejections[3]
        = 0;
    writer->results[0] = writer->results[1] = writer->results[2] = 0;
    return true;
}

static bool trace_emit(TraceWriter* writer, const DatagenGame* game)
{
    if (writer->games_in_shard == writer->shard_game_limit) {
        if (!trace_finish_shard(writer)) {
            return false;
        }
        writer->shard_index++;
        writer->games_in_shard = 0;
        if (!trace_open(writer)) {
            return false;
        }
    }

    FILE* file        = writer->file;
    char* opening_fen = cboard_to_fen((CBoard*)&game->opening_board);
    if (!opening_fen) {
        return false;
    }
    fprintf(file, "game %zu opening %zu result %s terminal %s moves %zu samples %zu\n",
            game->game_index, game->opening_index, result_name(game->result),
            terminal_name(game->terminal_reason), game->move_count, game->sample_count);
    fprintf(file, "opening_fen %s\n", opening_fen);
    free(opening_fen);
    fputs("moves", file);
    for (size_t i = 0; i < game->move_count; i++) {
        char move_string[6];
        move_to_uci_string(game->moves[i], move_string);
        fprintf(file, " %s", move_string);
    }
    fputc('\n', file);
    for (size_t i = 0; i < game->sample_count; i++) {
        const DatagenSample* sample = &game->samples[i];
        char* fen                   = cboard_to_fen((CBoard*)&sample->board);
        if (!fen) {
            return false;
        }
        fprintf(file, "sample %d %c %d %.1f %d %d %s\n", sample->ply,
                sample->side_to_move == WHITE ? 'w' : 'b', sample->score, sample->result,
                sample->completed_depth, sample->root_line_count, fen);
        free(fen);
        if (!binpack_append(&writer->binpack, &sample->board, sample->best_move, sample->score,
                            sample->ply, sample->result)) {
            return false;
        }
        writer->records_in_shard++;
        if (writer->records_in_shard == 1 || sample->score < writer->score_min)
            writer->score_min = sample->score;
        if (writer->records_in_shard == 1 || sample->score > writer->score_max)
            writer->score_max = sample->score;
        if (writer->records_in_shard == 1 || sample->ply < writer->ply_min)
            writer->ply_min = sample->ply;
        if (writer->records_in_shard == 1 || sample->ply > writer->ply_max)
            writer->ply_max = sample->ply;
    }
    for (RejectionReason reason = REJECT_TERMINAL; reason < REJECT_COUNT; reason++) {
        if (game->rejections[reason] > 0) {
            fprintf(file, "reject %s %" PRIu64 "\n", rejection_name(reason),
                    game->rejections[reason]);
        }
        writer->rejections[reason] += game->rejections[reason];
    }
    fputs("endgame\n\n", file);
    if (ferror(file)) {
        return false;
    }
    writer->games_in_shard++;
    writer->results[game->result]++;
    return true;
}

static bool trace_close(TraceWriter* writer)
{
    return !writer->file || trace_finish_shard(writer);
}

static void free_game(DatagenGame* game)
{
    free(game->moves);
    free(game->samples);
    *game = (DatagenGame) { 0 };
}

static bool run_game(const DatagenConfig* config, const OpeningSet* openings, size_t game_index,
                     TraceWriter* writer, size_t* rejected_games)
{
    uint64_t game_seed = derived_seed(config->root_seed, game_index);
    ranctx opening_random;
    raninit(&opening_random, game_seed);
    size_t opening_index
        = config->opening_first + (size_t)(ranval(&opening_random) % config->opening_count);

    CBoard board = openings->boards[opening_index];
    bool swap    = config->color_assignment == COLOR_SWAP
        || (config->color_assignment == COLOR_ALTERNATE && (game_index & 1U));
    if (swap) {
        CBoard transformed;
        swap_colors_and_ranks(&board, &transformed);
        board = transformed;
    }

    DatagenGame game = {
        .game_index    = game_index,
        .opening_index = opening_index,
        .opening_board = board,
        .result        = GAME_DRAW,
    };
    game.moves     = calloc((size_t)config->max_game_ply, sizeof(*game.moves));
    game.samples   = calloc((size_t)config->max_game_ply + 1, sizeof(*game.samples));
    uint64_t* keys = calloc((size_t)config->max_game_ply + 1, sizeof(*keys));
    if (!game.moves || !game.samples || !keys) {
        fprintf(stderr, "datagen worker failed to allocate game buffers\n");
        free(keys);
        free_game(&game);
        return false;
    }

    ranctx move_random;
    raninit(&move_random, derived_seed(game_seed, 0x4d4f5645ULL));
    ranctx sample_random;
    raninit(&sample_random, derived_seed(game_seed, config->sample_offset_seed));
    int sample_offset       = config->sample_interval == 1
        ? 0
        : (int)(ranval(&sample_random) % (uint64_t)config->sample_interval);
    int64_t first_sample_ply = (int64_t)config->sample_start_ply + sample_offset;

    PositionHistory history = { .keys = { board.zobrist_key }, .count = 1 };
    keys[0]                 = board.zobrist_key;
    bool complete           = false;
    size_t ply              = 0;
    for (; ply <= (size_t)config->max_game_ply; ply++) {
        MoveList legal_moves;
        init_move_list(&legal_moves);
        generate_legal_moves(&board, &legal_moves);
        bool scheduled = (int)ply >= first_sample_ply
            && (((int)ply - first_sample_ply) % config->sample_interval == 0);

        if (legal_moves.count == 0) {
            if (scheduled) {
                game.rejections[REJECT_TERMINAL]++;
            }
            game.terminal_reason = is_king_in_check(&board, board.side_to_move)
                ? TERMINAL_CHECKMATE
                : TERMINAL_STALEMATE;
            game.result          = game.terminal_reason == TERMINAL_CHECKMATE
                ? (board.side_to_move == WHITE ? GAME_BLACK_WIN : GAME_WHITE_WIN)
                : GAME_DRAW;
            complete             = true;
            break;
        }
        if (position_has_repetition(keys, ply + 1, board.zobrist_key)) {
            if (scheduled) {
                game.rejections[REJECT_TERMINAL]++;
            }
            game.terminal_reason = TERMINAL_REPETITION;
            game.result          = GAME_DRAW;
            complete             = true;
            break;
        }
        if (board.half_move_clock >= 100) {
            if (scheduled) {
                game.rejections[REJECT_TERMINAL]++;
            }
            game.terminal_reason = TERMINAL_MOVE_RULE;
            game.result          = GAME_DRAW;
            complete             = true;
            break;
        }
        if (ply == (size_t)config->max_game_ply) {
            break;
        }

        SearchLimits limits = { 0 };
        limits.node_limit   = config->nodes_per_move;
        limits.multipv      = (int)ply < config->early_ply_limit ? config->early_multipv : 1;
        SearchInput input   = {
            .board               = board,
            .position_history    = history,
            .limits              = limits,
            .suppress_uci_output = true,
        };
        SearchControl control;
        search_control_reset(&control, false);
        if (config->clear_hash_per_game && ply == 0) {
            clear_tt();
        }
        SearchResult search_result = search_run(&input, &control);

        if (scheduled) {
            if (is_king_in_check(&board, board.side_to_move)) {
                game.rejections[REJECT_IN_CHECK]++;
            } else if (search_result.completed_depth == 0 || search_result.root_line_count == 0) {
                game.rejections[REJECT_PARTIAL_SEARCH]++;
            } else if (search_result.score >= MATE_THRESHOLD
                       || search_result.score <= -MATE_THRESHOLD) {
                game.rejections[REJECT_MATE_SCORE]++;
            } else {
                DatagenSample* sample   = &game.samples[game.sample_count++];
                sample->board           = board;
                sample->ply             = (int)ply;
                sample->score           = search_result.score;
                sample->completed_depth = search_result.completed_depth;
                sample->root_line_count = search_result.root_line_count;
                sample->best_move       = search_result.best_move;
                sample->side_to_move    = board.side_to_move;
            }
        }

        if (search_result.completed_depth == 0 || search_result.best_move == MOVE_NONE) {
            (*rejected_games)++;
            free(keys);
            free_game(&game);
            return true;
        }

        Move selected = search_result.best_move;
        if ((int)ply < config->early_ply_limit && search_result.root_line_count > 1) {
            int candidate_count = config->early_multipv < search_result.root_line_count
                ? config->early_multipv
                : search_result.root_line_count;
            selected
                = search_result.root_lines[ranval(&move_random) % (uint64_t)candidate_count].move;
        }
        bool selected_is_legal = false;
        for (int i = 0; i < legal_moves.count; i++) {
            if (legal_moves.moves[i] == selected) {
                selected_is_legal = true;
                break;
            }
        }
        if (!selected_is_legal) {
            fprintf(stderr, "datagen search returned an illegal move at game %zu ply %zu\n",
                    game_index, ply);
            free(keys);
            free_game(&game);
            return false;
        }

        game.moves[game.move_count++] = selected;
        make_move(&board, selected);
        keys[ply + 1] = board.zobrist_key;
        add_position_history(&history, board.zobrist_key, board.half_move_clock);
    }

    if (!complete) {
        (*rejected_games)++;
        free(keys);
        free_game(&game);
        return true;
    }
    for (size_t i = 0; i < game.sample_count; i++) {
        game.samples[i].result = sample_result(game.result, game.samples[i].side_to_move);
    }
    bool emitted = trace_emit(writer, &game);
    free(keys);
    free_game(&game);
    return emitted;
}

static int run_worker(const DatagenConfig* config, const OpeningSet* openings, const char* output,
                      int worker_id)
{
    init_sliding_attacks();
    init_zobrist_keys();
    hc_eval_init();
    init_tt((size_t)config->hash_mb);

    TraceWriter writer = { 0 };
    if (snprintf(writer.output_base, sizeof(writer.output_base), "%s", output)
        >= (int)sizeof(writer.output_base)) {
        fprintf(stderr, "datagen output prefix is too long\n");
        free_tt();
        return 1;
    }
    snprintf(writer.openings_sha256, sizeof(writer.openings_sha256), "%s", config->openings_sha256);
    writer.worker_id        = worker_id;
    writer.worker_seed      = derived_seed(config->worker_seed_base, (uint64_t)worker_id);
    writer.shard_game_limit = config->shard_game_limit;
    writer.config           = config;
    if (!trace_open(&writer)) {
        free_tt();
        return 1;
    }

    size_t rejected_games = 0;
    int status            = 0;
    for (size_t game_index = (size_t)worker_id; game_index < config->games;
         game_index += (size_t)config->workers) {
        if (!run_game(config, openings, game_index, &writer, &rejected_games)) {
            status = 1;
            break;
        }
        fprintf(stderr, "datagen worker=%d game=%zu/%zu discarded=%zu\n", worker_id, game_index + 1,
                config->games, rejected_games);
        fflush(stderr);
    }
    if (status == 0) {
        if (!trace_close(&writer)) {
            status = 1;
        }
    } else {
        // A failed game write must never publish a complete shard manifest.
        if (writer.file) {
            fclose(writer.file);
        }
        if (writer.binpack.file) {
            binpack_close(&writer.binpack);
        }
    }
    free_tt();
    return status;
}

int datagen_main(int argc, char* argv[])
{
    const char* config_path = NULL;
    const char* output_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--config") && i + 1 < argc) {
            config_path = argv[++i];
        } else if (!strcmp(argv[i], "--output") && i + 1 < argc) {
            output_path = argv[++i];
        } else if (!strcmp(argv[i], "--help")) {
            print_usage(stdout);
            return 0;
        } else {
            print_usage(stderr);
            return 2;
        }
    }
    if (!config_path || !output_path) {
        print_usage(stderr);
        return 2;
    }

    DatagenConfig config;
    char error[256] = "";
    if (!parse_config(config_path, &config, error, sizeof(error))) {
        fprintf(stderr, "datagen: %s\n", error);
        return 1;
    }

    init_sliding_attacks();
    init_zobrist_keys();
    OpeningSet openings;
    if (!load_openings(&config, &openings, error, sizeof(error))) {
        fprintf(stderr, "datagen: %s\n", error);
        return 1;
    }

    pid_t* children = calloc((size_t)config.workers, sizeof(*children));
    if (!children) {
        fprintf(stderr, "datagen: out of memory for workers\n");
        free_openings(&openings);
        return 1;
    }
    int started = 0;
    int status  = 0;
    for (int worker = 0; worker < config.workers; worker++) {
        pid_t child = fork();
        if (child < 0) {
            fprintf(stderr, "datagen: fork failed: %s\n", strerror(errno));
            status = 1;
            break;
        }
        if (child == 0) {
            int worker_status = run_worker(&config, &openings, output_path, worker);
            free_openings(&openings);
            free(children);
            return worker_status;
        }
        children[started++] = child;
    }
    for (int i = 0; i < started; i++) {
        int child_status = 0;
        if (waitpid(children[i], &child_status, 0) < 0 || !WIFEXITED(child_status)
            || WEXITSTATUS(child_status) != 0) {
            status = 1;
        }
    }
    free(children);
    free_openings(&openings);
    return status;
}
