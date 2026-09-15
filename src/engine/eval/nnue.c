#include "engine/eval/nnue.h"

#include "chess/core/bitboard.h"
#include "engine/eval/nnue_features.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    PNUE_HEADER_BYTES     = 188,
    PNUE_FEATURE_COUNT    = 22528,
    PNUE_DENSE_INPUTS     = 512,
    PNUE_HIDDEN           = 32,
    PNUE_ACTIVATION_SCALE = 127,
    PNUE_DENSE_SCALE      = 64,
    PNUE_EVAL_SCALE       = 841,
    PNUE_MATE_THRESHOLD   = 199999000,
};

/** Returns the exact V1 payload length, excluding the fixed PNUE header. */
static size_t expected_payload_bytes(void);

typedef struct {
    const char* architecture_id;
    const char* feature_id;
    const char* quantization_id;
    uint32_t eval_scale_cp;
    uint32_t dimensions[8];
    NnueFeatureSet feature_set;
    size_t (*payload_bytes)(void);
} NnueProfile;

static const NnueProfile V1_PROFILE = {
    .architecture_id = "prophet-nnue-v1",
    .feature_id      = "halfkav2_hm-v1",
    .quantization_id = "QA127_QW64_V1",
    .eval_scale_cp   = PNUE_EVAL_SCALE,
    .dimensions      = { PNUE_FEATURE_COUNT, NNUE_ACCUMULATOR_SIZE, PNUE_DENSE_INPUTS, PNUE_HIDDEN,
                         PNUE_HIDDEN, PNUE_HIDDEN, PNUE_HIDDEN, 1 },
    .feature_set     = NNUE_FEATURE_HALFKAV2_HM_V1,
    .payload_bytes   = expected_payload_bytes,
};

static const NnueProfile* const SUPPORTED_PROFILES[] = { &V1_PROFILE };

typedef struct {
    const NnueProfile* profile;
    int16_t feature_bias[NNUE_ACCUMULATOR_SIZE];
    int16_t* feature_weight;
    int32_t dense_1_bias[PNUE_HIDDEN];
    int8_t dense_1_weight[PNUE_HIDDEN][PNUE_DENSE_INPUTS];
    int32_t dense_2_bias[PNUE_HIDDEN];
    int8_t dense_2_weight[PNUE_HIDDEN][PNUE_HIDDEN];
    int32_t output_bias;
    int8_t output_weight[PNUE_HIDDEN];
    char model_identifier[65];
} NnueNetwork;

static NnueNetwork* active_network;

/** Writes one bounded loader error message when the caller supplied storage. */
static void set_error(char* error, size_t error_size, const char* message)
{
    if (error && error_size) {
        snprintf(error, error_size, "%s", message);
    }
}

/** Decodes one unsigned 32-bit little-endian value without struct casting. */
static uint32_t read_u32_le(const uint8_t* bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16)
        | ((uint32_t)bytes[3] << 24);
}

/** Decodes one unsigned 64-bit little-endian value without struct casting. */
static uint64_t read_u64_le(const uint8_t* bytes)
{
    uint64_t value = 0;
    for (int index = 7; index >= 0; index--) {
        value = (value << 8) | bytes[index];
    }
    return value;
}

/** Decodes one signed 16-bit little-endian payload value. */
static int16_t read_i16_le(const uint8_t* bytes)
{
    return (int16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

/** Decodes one signed 32-bit little-endian payload value. */
static int32_t read_i32_le(const uint8_t* bytes)
{
    return (int32_t)read_u32_le(bytes);
}

/** Returns the exact V1 payload length, excluding the fixed PNUE header. */
static size_t expected_payload_bytes(void)
{
    return NNUE_ACCUMULATOR_SIZE * sizeof(int16_t)
        + (size_t)PNUE_FEATURE_COUNT * NNUE_ACCUMULATOR_SIZE * sizeof(int16_t)
        + PNUE_HIDDEN * sizeof(int32_t) + PNUE_HIDDEN * PNUE_DENSE_INPUTS * sizeof(int8_t)
        + PNUE_HIDDEN * sizeof(int32_t) + PNUE_HIDDEN * PNUE_HIDDEN * sizeof(int8_t)
        + sizeof(int32_t) + PNUE_HIDDEN * sizeof(int8_t);
}

/** Validates one NUL-padded ASCII identifier. */
static bool fixed_identifier(const uint8_t* bytes, const char* expected)
{
    size_t expected_length = strlen(expected);
    if (expected_length == 0 || expected_length > 32 || memcmp(bytes, expected, expected_length)) {
        return false;
    }
    for (size_t index = expected_length; index < 32; index++) {
        if (bytes[index] != 0) {
            return false;
        }
    }
    return true;
}

/** Selects the locally supported profile that exactly matches a PNUE header. */
static const NnueProfile* profile_for_header(const uint8_t header[PNUE_HEADER_BYTES],
                                             uint64_t payload_bytes)
{
    for (size_t profile_index = 0;
         profile_index < sizeof(SUPPORTED_PROFILES) / sizeof(SUPPORTED_PROFILES[0]);
         profile_index++) {
        const NnueProfile* profile = SUPPORTED_PROFILES[profile_index];
        if (payload_bytes != profile->payload_bytes()
            || read_u32_le(header + 16) != profile->eval_scale_cp
            || !fixed_identifier(header + 60, profile->architecture_id)
            || !fixed_identifier(header + 92, profile->feature_id)
            || !fixed_identifier(header + 124, profile->quantization_id)) {
            continue;
        }
        bool dimensions_match = true;
        for (size_t index = 0; index < sizeof(profile->dimensions) / sizeof(profile->dimensions[0]);
             index++) {
            dimensions_match &= read_u32_le(header + 20 + index * 4) == profile->dimensions[index];
        }
        if (dimensions_match) {
            return profile;
        }
    }
    return NULL;
}

/** Frees all heap storage owned by one staged or active network. */
static void free_network(NnueNetwork* network)
{
    if (!network) {
        return;
    }
    free(network->feature_weight);
    free(network);
}

/** Inserts one magnitude into a descending fixed-size top-32 list. */
static void insert_top_abs(int64_t values[32], int64_t value)
{
    if (value <= values[31]) {
        return;
    }
    int index = 31;
    while (index > 0 && value > values[index - 1]) {
        values[index] = values[index - 1];
        index--;
    }
    values[index] = value;
}

/** Checks that this profile cannot overflow its accumulator or scalar arithmetic. */
static bool validate_bounds(const NnueNetwork* network)
{
    for (int output = 0; output < NNUE_ACCUMULATOR_SIZE; output++) {
        int64_t largest[32] = { 0 };
        for (int feature = 0; feature < PNUE_FEATURE_COUNT; feature++) {
            int64_t value
                = network->feature_weight[(size_t)feature * NNUE_ACCUMULATOR_SIZE + output];
            insert_top_abs(largest, value < 0 ? -value : value);
        }
        int64_t bound = network->feature_bias[output];
        bound         = bound < 0 ? -bound : bound;
        for (int index = 0; index < 32; index++) {
            bound += largest[index];
        }
        if (bound > INT16_MAX) {
            return false;
        }
    }

    const int32_t* biases[]
        = { network->dense_1_bias, network->dense_2_bias, &network->output_bias };
    const int8_t* weights[] = { &network->dense_1_weight[0][0], &network->dense_2_weight[0][0],
                                network->output_weight };
    const int inputs[]      = { PNUE_DENSE_INPUTS, PNUE_HIDDEN, PNUE_HIDDEN };
    const int outputs[]     = { PNUE_HIDDEN, PNUE_HIDDEN, 1 };
    int64_t final_bound     = 0;
    for (int layer = 0; layer < 3; layer++) {
        for (int output = 0; output < outputs[layer]; output++) {
            int64_t bound = biases[layer][output];
            bound         = bound < 0 ? -bound : bound;
            for (int input = 0; input < inputs[layer]; input++) {
                int64_t weight = weights[layer][output * inputs[layer] + input];
                bound += PNUE_ACTIVATION_SCALE * (weight < 0 ? -weight : weight);
            }
            if (bound > INT32_MAX) {
                return false;
            }
            if (layer == 2) {
                final_bound = bound;
            }
        }
    }
    return final_bound <= INT64_MAX / PNUE_EVAL_SCALE
        && final_bound * PNUE_EVAL_SCALE / (PNUE_ACTIVATION_SCALE * PNUE_DENSE_SCALE)
        < PNUE_MATE_THRESHOLD;
}

/** Decodes the V1 payload into a staged network in its documented tensor order. */
static bool decode_payload(const uint8_t* payload, NnueNetwork* network)
{
    size_t offset = 0;
    for (int index = 0; index < NNUE_ACCUMULATOR_SIZE; index++, offset += 2) {
        network->feature_bias[index] = read_i16_le(payload + offset);
    }
    for (size_t index = 0; index < (size_t)PNUE_FEATURE_COUNT * NNUE_ACCUMULATOR_SIZE;
         index++, offset += 2) {
        network->feature_weight[index] = read_i16_le(payload + offset);
    }
    for (int index = 0; index < PNUE_HIDDEN; index++, offset += 4) {
        network->dense_1_bias[index] = read_i32_le(payload + offset);
    }
    memcpy(network->dense_1_weight, payload + offset, sizeof(network->dense_1_weight));
    offset += sizeof(network->dense_1_weight);
    for (int index = 0; index < PNUE_HIDDEN; index++, offset += 4) {
        network->dense_2_bias[index] = read_i32_le(payload + offset);
    }
    memcpy(network->dense_2_weight, payload + offset, sizeof(network->dense_2_weight));
    offset += sizeof(network->dense_2_weight);
    network->output_bias = read_i32_le(payload + offset);
    offset += 4;
    memcpy(network->output_weight, payload + offset, sizeof(network->output_weight));
    return offset + sizeof(network->output_weight) == expected_payload_bytes();
}

bool nnue_load_file(const char* path, char* error, size_t error_size)
{
    set_error(error, error_size, "invalid NNUE file");
    if (!path || !*path) {
        set_error(error, error_size, "EvalFile requires a path");
        return false;
    }
    FILE* file = fopen(path, "rb");
    if (!file) {
        set_error(error, error_size, "cannot open EvalFile");
        return false;
    }
    uint8_t header[PNUE_HEADER_BYTES];
    bool okay = fread(header, 1, sizeof(header), file) == sizeof(header);
    if (!okay || fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        set_error(error, error_size, "truncated PNUE header");
        return false;
    }
    long file_length = ftell(file);
    if (file_length < 0 || fseek(file, PNUE_HEADER_BYTES, SEEK_SET) != 0) {
        fclose(file);
        set_error(error, error_size, "cannot read PNUE file");
        return false;
    }
    bool header_valid = !memcmp(header, "PNUE", 4) && read_u32_le(header + 4) == 1
        && read_u32_le(header + 8) == 0x01020304U && read_u32_le(header + 12) == 1;
    const uint64_t payload_bytes = read_u64_le(header + 52);
    const NnueProfile* profile   = header_valid ? profile_for_header(header, payload_bytes) : NULL;
    header_valid = profile != NULL && (uint64_t)file_length == PNUE_HEADER_BYTES + payload_bytes;
    NnueNetwork* candidate = NULL;
    if (header_valid) {
        candidate    = calloc(1, sizeof(*candidate));
        header_valid = candidate != NULL;
    }
    if (header_valid) {
        candidate->profile = profile;
        candidate->feature_weight
            = malloc((size_t)payload_bytes - NNUE_ACCUMULATOR_SIZE * 2 - PNUE_HIDDEN * 4
                     - PNUE_HIDDEN * PNUE_DENSE_INPUTS - PNUE_HIDDEN * 4 - PNUE_HIDDEN * PNUE_HIDDEN
                     - 4 - PNUE_HIDDEN);
        header_valid = candidate->feature_weight != NULL;
        for (size_t index = 0; header_valid && index < 32; index++) {
            snprintf(candidate->model_identifier + index * 2,
                     sizeof(candidate->model_identifier) - index * 2, "%02x", header[156 + index]);
        }
    }
    uint8_t* payload = NULL;
    if (header_valid) {
        payload      = malloc((size_t)payload_bytes);
        header_valid = payload != NULL
            && fread(payload, 1, (size_t)payload_bytes, file) == payload_bytes && fgetc(file) == EOF
            && decode_payload(payload, candidate) && validate_bounds(candidate);
    }
    free(payload);
    fclose(file);
    if (!header_valid) {
        free_network(candidate);
        set_error(error, error_size, "invalid or unsafe V1 PNUE file");
        return false;
    }
    NnueNetwork* previous = active_network;
    active_network        = candidate;
    free_network(previous);
    return true;
}

void nnue_unload(void)
{
    free_network(active_network);
    active_network = NULL;
}

bool nnue_is_loaded(void)
{
    return active_network != NULL;
}

const char* nnue_architecture_id(void)
{
    return active_network ? active_network->profile->architecture_id : "";
}

const char* nnue_model_identifier(void)
{
    return active_network ? active_network->model_identifier : "";
}

/** Rebuilds one king-perspective accumulator from active feature rows. */
static bool refresh_perspective(const CBoard* board, Color perspective, int16_t output[256])
{
    uint16_t features[NNUE_MAX_ACTIVE_FEATURES];
    size_t count = 0;
    if (!active_network
        || !nnue_generate_features(board, active_network->profile->feature_set, perspective,
                                   features, NNUE_MAX_ACTIVE_FEATURES, &count)) {
        return false;
    }
    for (int output_index = 0; output_index < NNUE_ACCUMULATOR_SIZE; output_index++) {
        int32_t value = active_network->feature_bias[output_index];
        for (size_t feature = 0; feature < count; feature++) {
            value
                += active_network->feature_weight[(size_t)features[feature] * NNUE_ACCUMULATOR_SIZE
                                                  + output_index];
        }
        if (value < INT16_MIN || value > INT16_MAX) {
            return false;
        }
        output[output_index] = (int16_t)value;
    }
    return true;
}

bool nnue_refresh_accumulator(const CBoard* board, NnueAccumulator* accumulator)
{
    if (!accumulator) {
        return false;
    }
    accumulator->valid[WHITE] = refresh_perspective(board, WHITE, accumulator->values[WHITE]);
    accumulator->valid[BLACK] = refresh_perspective(board, BLACK, accumulator->values[BLACK]);
    return accumulator->valid[WHITE] && accumulator->valid[BLACK];
}

/** Returns the color and piece at square when that square is occupied. */
static bool piece_on_square(const CBoard* board, Square square, Color* color, PieceType* piece)
{
    *piece = cboard_get_piece_at_square(board, square);
    if (*piece == NO_PIECE) {
        return false;
    }
    *color = bitboard_is_bit_set(board->occupancy_bbs[WHITE], square) ? WHITE : BLACK;
    return true;
}

/** Updates one perspective by removing and adding the pieces that changed squares. */
static bool update_perspective(const CBoard* parent, const CBoard* child, Color perspective,
                               const int16_t source[256], int16_t destination[256])
{
    if (!active_network || !source || !destination) {
        return false;
    }
    if (parent->piece_bbs[perspective][KING] != child->piece_bbs[perspective][KING]) {
        return refresh_perspective(child, perspective, destination);
    }
    int32_t totals[NNUE_ACCUMULATOR_SIZE];
    for (int index = 0; index < NNUE_ACCUMULATOR_SIZE; index++) {
        totals[index] = source[index];
    }
    for (Square square = A1; square < NO_SQUARE; square++) {
        Color parent_color = WHITE, child_color = WHITE;
        PieceType parent_piece = NO_PIECE, child_piece = NO_PIECE;
        bool had_parent = piece_on_square(parent, square, &parent_color, &parent_piece);
        bool has_child  = piece_on_square(child, square, &child_color, &child_piece);
        if (had_parent == has_child
            && (!had_parent || (parent_color == child_color && parent_piece == child_piece))) {
            continue;
        }
        uint16_t feature;
        if (had_parent
            && !nnue_feature_index_for_piece(parent, active_network->profile->feature_set,
                                             perspective, parent_color, parent_piece, square,
                                             &feature)) {
            return false;
        }
        for (int index = 0; had_parent && index < NNUE_ACCUMULATOR_SIZE; index++) {
            totals[index]
                -= active_network->feature_weight[(size_t)feature * NNUE_ACCUMULATOR_SIZE + index];
        }
        if (has_child
            && !nnue_feature_index_for_piece(child, active_network->profile->feature_set,
                                             perspective, child_color, child_piece, square,
                                             &feature)) {
            return false;
        }
        for (int index = 0; has_child && index < NNUE_ACCUMULATOR_SIZE; index++) {
            totals[index]
                += active_network->feature_weight[(size_t)feature * NNUE_ACCUMULATOR_SIZE + index];
        }
    }
    for (int index = 0; index < NNUE_ACCUMULATOR_SIZE; index++) {
        if (totals[index] < INT16_MIN || totals[index] > INT16_MAX) {
            return false;
        }
        destination[index] = (int16_t)totals[index];
    }
    return true;
}

bool nnue_update_accumulator(const CBoard* parent, const CBoard* child,
                             const NnueAccumulator* parent_accumulator,
                             NnueAccumulator* child_accumulator)
{
    if (!parent || !child || !parent_accumulator || !child_accumulator
        || !parent_accumulator->valid[WHITE] || !parent_accumulator->valid[BLACK]) {
        return false;
    }
    child_accumulator->valid[WHITE] = update_perspective(
        parent, child, WHITE, parent_accumulator->values[WHITE], child_accumulator->values[WHITE]);
    child_accumulator->valid[BLACK] = update_perspective(
        parent, child, BLACK, parent_accumulator->values[BLACK], child_accumulator->values[BLACK]);
    return child_accumulator->valid[WHITE] && child_accumulator->valid[BLACK];
}

int nnue_evaluate_accumulator(const CBoard* board, const NnueAccumulator* accumulator)
{
    if (!active_network || !board || !accumulator || !accumulator->valid[WHITE]
        || !accumulator->valid[BLACK]) {
        return 0;
    }
    int activation[PNUE_DENSE_INPUTS];
    Color first  = board->side_to_move;
    Color second = color_opposite(first);
    for (int index = 0; index < NNUE_ACCUMULATOR_SIZE; index++) {
        int first_value                           = accumulator->values[first][index];
        int second_value                          = accumulator->values[second][index];
        activation[index]                         = first_value < 0 ? 0
            : first_value > PNUE_ACTIVATION_SCALE                   ? PNUE_ACTIVATION_SCALE
                                                                    : first_value;
        activation[index + NNUE_ACCUMULATOR_SIZE] = second_value < 0 ? 0
            : second_value > PNUE_ACTIVATION_SCALE                   ? PNUE_ACTIVATION_SCALE
                                                                     : second_value;
    }
    int hidden_1[PNUE_HIDDEN];
    int hidden_2[PNUE_HIDDEN];
    // C17 signed division truncates toward zero, as required by the integer contract.
    for (int output = 0; output < PNUE_HIDDEN; output++) {
        int64_t sum = active_network->dense_1_bias[output];
        for (int input = 0; input < PNUE_DENSE_INPUTS; input++) {
            sum += (int64_t)activation[input] * active_network->dense_1_weight[output][input];
        }
        int value        = (int)(sum / PNUE_DENSE_SCALE);
        hidden_1[output] = value < 0        ? 0
            : value > PNUE_ACTIVATION_SCALE ? PNUE_ACTIVATION_SCALE
                                            : value;
    }
    for (int output = 0; output < PNUE_HIDDEN; output++) {
        int64_t sum = active_network->dense_2_bias[output];
        for (int input = 0; input < PNUE_HIDDEN; input++) {
            sum += (int64_t)hidden_1[input] * active_network->dense_2_weight[output][input];
        }
        int value        = (int)(sum / PNUE_DENSE_SCALE);
        hidden_2[output] = value < 0        ? 0
            : value > PNUE_ACTIVATION_SCALE ? PNUE_ACTIVATION_SCALE
                                            : value;
    }
    int64_t final_sum = active_network->output_bias;
    for (int input = 0; input < PNUE_HIDDEN; input++) {
        final_sum += (int64_t)hidden_2[input] * active_network->output_weight[input];
    }
    return (int)(final_sum * PNUE_EVAL_SCALE / (PNUE_ACTIVATION_SCALE * PNUE_DENSE_SCALE));
}

int nnue_evaluate_cboard(const CBoard* board)
{
    NnueAccumulator accumulator = { 0 };
    return nnue_refresh_accumulator(board, &accumulator)
        ? nnue_evaluate_accumulator(board, &accumulator)
        : 0;
}
