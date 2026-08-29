#ifndef PROPHET_NNUE_HALFKAV2_HM_V1_VECTORS_H
#define PROPHET_NNUE_HALFKAV2_HM_V1_VECTORS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char* name;
    const char* fen;
    const uint16_t* white_features;
    size_t white_count;
    const uint16_t* black_features;
    size_t black_count;
} NnueContractFixture;

static const char NNUE_CONTRACT_SHA256[] = "29ea693ec0d5296082b4449002cba2bde07beca594d44e39caabadf370c0f515";

static const uint16_t nnue_contract_0_white[] = {8, 9, 10, 11, 12, 13, 14, 15, 65, 70, 130, 133, 192, 199, 259, 368, 369, 370, 371, 372, 373, 374, 375, 441, 446, 506, 509, 568, 575, 635, 644, 700};
static const uint16_t nnue_contract_0_black[] = {8, 9, 10, 11, 12, 13, 14, 15, 65, 70, 130, 133, 192, 199, 259, 368, 369, 370, 371, 372, 373, 374, 375, 441, 446, 506, 509, 568, 575, 635, 644, 700};

static const uint16_t nnue_contract_1_white[] = {8461, 8711, 8809, 9010, 9116, 9144};
static const uint16_t nnue_contract_1_black[] = {2134, 2317, 2482, 2744, 2759, 2787};

static const uint16_t nnue_contract_2_white[] = {8458, 8704, 9005, 9116, 9144};
static const uint16_t nnue_contract_2_black[] = {2322, 2485, 2751, 2759, 2787};

static const uint16_t nnue_contract_3_white[] = {724, 975, 1067, 1264, 1349, 1402};
static const uint16_t nnue_contract_3_black[] = {724, 911, 1067, 1328, 1349, 1402};

static const uint16_t nnue_contract_4_white[] = {724, 911, 1067, 1328, 1349, 1402};
static const uint16_t nnue_contract_4_black[] = {724, 975, 1067, 1264, 1349, 1402};

static const uint16_t nnue_contract_5_white[] = {6347, 6997, 7012};
static const uint16_t nnue_contract_5_black[] = {8819, 9116, 9133};

static const uint16_t nnue_contract_6_white[] = {192, 199, 568, 575, 644, 700};
static const uint16_t nnue_contract_6_black[] = {192, 199, 568, 575, 644, 700};

static const uint16_t nnue_contract_7_white[] = {36, 355, 644, 700};
static const uint16_t nnue_contract_7_black[] = {27, 348, 644, 700};

static const uint16_t nnue_contract_8_white[] = {2239, 2302, 2365, 2428, 2501, 2564, 2627, 2694, 2759, 2808};
static const uint16_t nnue_contract_8_black[] = {2234, 2299, 2364, 2425, 2496, 2561, 2626, 2691, 2759, 2808};

static const NnueContractFixture NNUE_CONTRACT_FIXTURES[] = {
    {"initial_position", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", nnue_contract_0_white, 32, nnue_contract_0_black, 32},
    {"horizontal_mirror_d_file", "7k/5r2/6p1/8/3K4/8/2P5/Q7 w - - 0 1", nnue_contract_1_white, 6, nnue_contract_1_black, 6},
    {"horizontal_no_mirror_e_file", "k7/8/5r2/8/4K3/8/2P5/Q7 w - - 0 1", nnue_contract_2_white, 5, nnue_contract_2_black, 5},
    {"vertical_color_swap_base", "5k2/7r/4p3/8/8/3P4/Q7/2K5 w - - 0 1", nnue_contract_3_white, 6, nnue_contract_3_black, 6},
    {"vertical_color_swap_mirror", "2k5/q7/3p4/8/8/4P3/7R/5K2 b - - 0 1", nnue_contract_4_white, 6, nnue_contract_4_black, 6},
    {"sparse_endgame", "8/8/8/3k4/8/2K5/4P3/8 w - - 0 1", nnue_contract_5_white, 3, nnue_contract_5_black, 3},
    {"castling_position", "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", nnue_contract_6_white, 6, nnue_contract_6_black, 6},
    {"en_passant_position", "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", nnue_contract_7_white, 4, nnue_contract_7_black, 4},
    {"promotion_piece_types", "NBRQ3k/8/8/8/8/8/8/Kqnbr3 w - - 0 1", nnue_contract_8_white, 10, nnue_contract_8_black, 10},
};

#define NNUE_CONTRACT_FIXTURE_COUNT (sizeof(NNUE_CONTRACT_FIXTURES) / sizeof(NNUE_CONTRACT_FIXTURES[0]))

#endif // PROPHET_NNUE_HALFKAV2_HM_V1_VECTORS_H
