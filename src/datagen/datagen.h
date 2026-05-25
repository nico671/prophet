#ifndef DATAGEN_H
#define DATAGEN_H
#include <stdint.h>
// Force the compiler to pack this struct tightly (no padding bytes)
#pragma pack(push, 1)
typedef struct {
    uint64_t pieces[6]; // Bitboards: Pawns, Knights, Bishops, Rooks, Queens, Kings
    uint64_t colors[2]; // Bitboards: White pieces, Black pieces
    uint8_t side_to_move; // 0 = White, 1 = Black
    uint8_t castling_rights; // 4-bit mask for KQkq
    uint8_t ep_square; // En passant square (0-63), or 255 if none
    int16_t search_score; // Score in centipawns (from side-to-move's perspective)
    int8_t game_result; // 0 = Loss, 1 = Draw, 2 = Win (from side-to-move's perspective)
} BinaryTrainingRecord;
#pragma pack(pop)

void generate_nnue_training_data(int num_games, int start_index, const char* output_file);

typedef struct {
    int search_depth;
    double temperature;
    int max_plies;
    int adjudicate_score_cp;
    int adjudicate_plies;
} DatagenConfig;

DatagenConfig datagen_default_config(void);

void generate_nnue_training_data_with_config(int num_games, int start_index, const char* output_file,
    const DatagenConfig* config);

#endif // DATAGEN_H