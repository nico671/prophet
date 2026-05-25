#include "datagen/datagen.h"
#include "engine/engine.h"
#include "movegen/move.h"
#include "movegen/move_make.h"
#include "movegen/movegen.h"
#include "search/search.h"
#include "utils/prng.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_MOVES 256
#define SELFPLAY_MAX_PLIES 512
#define SELFPLAY_SEARCH_DEPTH 6
#define SELFPLAY_TEMPERATURE 50.0
#define SELFPLAY_ADJUDICATE_SCORE_CP 1200
#define SELFPLAY_ADJUDICATE_PLIES 6

typedef enum {
    GAME_LOSS = 0,
    GAME_DRAW = 1,
    GAME_WIN = 2,
} GameResult;

static void fill_training_record(BinaryTrainingRecord* record, const CBoard* board, int score_cp)
{
    for (int pt = PAWN; pt <= KING; pt++) {
        record->pieces[pt - 1] = board->piece_bbs[WHITE][pt] | board->piece_bbs[BLACK][pt];
    }
    record->colors[WHITE] = board->occupancy_bbs[WHITE];
    record->colors[BLACK] = board->occupancy_bbs[BLACK];
    record->side_to_move = (uint8_t)board->side_to_move;
    record->castling_rights = board->castling_rights;
    record->ep_square = board->ep_square;
    record->search_score = (int16_t)score_cp;
    record->game_result = GAME_DRAW;
}

static GameResult result_for_side(GameResult game_result, Color side_to_move)
{
    if (game_result == GAME_DRAW) {
        return GAME_DRAW;
    }
    if (game_result == GAME_WIN) {
        return side_to_move == WHITE ? GAME_WIN : GAME_LOSS;
    }
    return side_to_move == WHITE ? GAME_LOSS : GAME_WIN;
}

DatagenConfig datagen_default_config(void)
{
    DatagenConfig config = {
        .search_depth = SELFPLAY_SEARCH_DEPTH,
        .temperature = SELFPLAY_TEMPERATURE,
        .max_plies = SELFPLAY_MAX_PLIES,
        .adjudicate_score_cp = SELFPLAY_ADJUDICATE_SCORE_CP,
        .adjudicate_plies = SELFPLAY_ADJUDICATE_PLIES,
    };
    return config;
}

static int datagen_search_score(CBoard* board, int depth)
{
    SearchLimits limits = { 0 };
    limits.depth_limit = depth;

    SearchReportUpdate update = { 0 };
    if (!engine_search_sync(board, &limits, &update)) {
        return 0;
    }

    return update.score_cp;
}

Move get_softmax_move(CBoard* board, int depth, double temperature, int* out_best_score)
{
    MoveList move_list;
    init_move_list(&move_list);
    generate_legal_moves(board, &move_list);

    if (move_list.count == 0) {
        if (out_best_score) {
            *out_best_score = 0;
        }
        return MOVE_NONE;
    }

    int scores[MAX_MOVES];
    int best_score = -MATE_SCORE;
    int best_index = 0;

    int search_depth = depth - 1;
    if (search_depth < 1) {
        search_depth = 1;
    }

    for (int i = 0; i < move_list.count; i++) {
        Move move = move_list.moves[i];
        UndoInfo undo_info = make_move(board, move);
        int score = -datagen_search_score(board, search_depth);
        unmake_move(board, move, undo_info);
        scores[i] = score;

        if (score > best_score) {
            best_score = score;
            best_index = i;
        }
    }

    if (out_best_score) {
        *out_best_score = best_score;
    }

    if (temperature <= 1e-6) {
        return move_list.moves[best_index];
    }

    static bool rng_initialized = false;
    static ranctx rng;
    if (!rng_initialized) {
        raninit(&rng, (uint64_t)time(NULL));
        rng_initialized = true;
    }

    double max_score = (double)best_score;
    double weights[MAX_MOVES];
    double weight_sum = 0.0;
    for (int i = 0; i < move_list.count; i++) {
        double scaled = ((double)scores[i] - max_score) / temperature;
        double weight = exp(scaled);
        weights[i] = weight;
        weight_sum += weight;
    }

    if (weight_sum <= 0.0) {
        return move_list.moves[best_index];
    }

    uint64_t rand_val = ranval(&rng);
    double rand_unit = (double)rand_val / (double)UINT64_MAX;
    double target = rand_unit * weight_sum;
    double cumulative = 0.0;
    for (int i = 0; i < move_list.count; i++) {
        cumulative += weights[i];
        if (target <= cumulative) {
            return move_list.moves[i];
        }
    }

    return move_list.moves[move_list.count - 1];
}

void generate_nnue_training_data(int num_games, int start_index, const char* output_file)
{
    DatagenConfig config = datagen_default_config();
    generate_nnue_training_data_with_config(num_games, start_index, output_file, &config);
}

void generate_nnue_training_data_with_config(int num_games, int start_index, const char* output_file,
    const DatagenConfig* config)
{
    DatagenConfig effective_config = config ? *config : datagen_default_config();
    printf("Generating NNUE training data for %d games starting from %d and writing to %s...\n", num_games, start_index, output_file);
    printf("Datagen config: depth=%d temp=%.2f max_plies=%d adjudicate_cp=%d adjudicate_plies=%d\n",
        effective_config.search_depth, effective_config.temperature, effective_config.max_plies,
        effective_config.adjudicate_score_cp, effective_config.adjudicate_plies);

    // initialize engine
    engine_init();

    FILE* openings = fopen("/Users/nicocarbone/Documents/dev/prophet/artifacts/UHO_4060_v4.epd", "r");
    if (!openings) {
        printf("Failed to open %s\n", "/Users/nicocarbone/Documents/dev/prophet/artifacts/UHO_4060_v4.epd");
        return;
    }

    FILE* output = fopen(output_file, "wb");
    if (!output) {
        printf("Failed to open %s\n", output_file);
        fclose(openings);
        return;
    }

    char line_buffer[1024];
    int total_epd_lines = 4060; // Size of UHO_4060_v4.epd

    // Calculate the actual line to start on, wrapping around if necessary
    int lines_to_skip = start_index % total_epd_lines;

    // skip ahead to the designated starting line
    for (int i = 0; i < lines_to_skip; i++) {
        if (fgets(line_buffer, sizeof(line_buffer), openings) == NULL) {
            // Safety net: if the file is unexpectedly short, reset to the top
            rewind(openings);
            break;
        }
    }
    // data generation loop
    for (int game_index = 0; game_index < num_games; game_index++) {
        engine_new_game();
        if (fgets(line_buffer, sizeof(line_buffer), openings) == NULL) {
            // If we reach the end of the file, wrap around to the beginning
            rewind(openings);
            if (fgets(line_buffer, sizeof(line_buffer), openings) == NULL) {
                printf("Failed to read from %s after rewinding\n", "/Users/nicocarbone/Documents/dev/prophet/artifacts/UHO_4060_v4.epd");
                break;
            }
        }

        line_buffer[strcspn(line_buffer, "\n")] = 0;
        if (!engine_set_position_fen(line_buffer)) {
            break;
        }

        CBoard board;
        if (!engine_copy_board(&board)) {
            break;
        }

        int max_plies = effective_config.max_plies > 0 ? effective_config.max_plies : SELFPLAY_MAX_PLIES;
        if (max_plies > SELFPLAY_MAX_PLIES) {
            max_plies = SELFPLAY_MAX_PLIES;
        }

        BinaryTrainingRecord records[SELFPLAY_MAX_PLIES];
        int record_count = 0;
        GameResult game_result = GAME_DRAW;
        int adjudicate_winner = -1;
        int adjudicate_streak = 0;

        for (int ply = 0; ply < max_plies; ply++) {
            MoveList moves;
            init_move_list(&moves);
            generate_legal_moves(&board, &moves);

            if (moves.count == 0) {
                if (is_king_in_check(&board, board.side_to_move)) {
                    game_result = GAME_LOSS; // side to move is checkmated
                } else {
                    game_result = GAME_DRAW; // stalemate
                }
                break;
            }

            if (board.half_move_clock >= 100) {
                game_result = GAME_DRAW;
                break;
            }

            int best_score = 0;
            Move move = get_softmax_move(&board, effective_config.search_depth, effective_config.temperature, &best_score);
            if (move == MOVE_NONE) {
                game_result = GAME_DRAW;
                break;
            }

            if (effective_config.adjudicate_score_cp > 0 && effective_config.adjudicate_plies > 0) {
                if (abs(best_score) >= effective_config.adjudicate_score_cp) {
                    int winner = best_score > 0 ? board.side_to_move : (int)(1 - board.side_to_move);
                    if (winner == adjudicate_winner) {
                        adjudicate_streak++;
                    } else {
                        adjudicate_winner = winner;
                        adjudicate_streak = 1;
                    }

                    if (adjudicate_streak >= effective_config.adjudicate_plies) {
                        game_result = winner == WHITE ? GAME_WIN : GAME_LOSS;
                        break;
                    }
                } else {
                    adjudicate_winner = -1;
                    adjudicate_streak = 0;
                }
            }

            if (record_count < SELFPLAY_MAX_PLIES) {
                fill_training_record(&records[record_count], &board, best_score);
                record_count++;
            }

            UndoInfo undo_info = make_move(&board, move);
            (void)undo_info;
        }

        for (int i = 0; i < record_count; i++) {
            GameResult side_result = result_for_side(game_result, (Color)records[i].side_to_move);
            records[i].game_result = (int8_t)side_result;
        }

        if (record_count > 0) {
            size_t written = fwrite(records, sizeof(BinaryTrainingRecord), (size_t)record_count, output);
            if (written != (size_t)record_count) {
                // printf("Failed to write records for game %d\n", game_index);
                break;
            }
        }
        // print progress and specific game result
        printf("Generated data for game %d/%d (%.2f%%) - Result: %s, Plies: %d\n", game_index + 1, num_games,
            ((double)(game_index + 1) / (double)num_games) * 100.0,
            game_result == GAME_WIN ? "Win" : (game_result == GAME_LOSS ? "Loss" : "Draw"), record_count);
    }

    // cleanup cleanup everybody cleanup
    fclose(openings);
    fclose(output);
}