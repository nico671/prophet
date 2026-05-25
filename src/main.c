
#include <stdio.h>

#include "board/cboard.h"
#include "datagen/datagen.h"
#include "engine/engine.h"
#include "nnue/nnue.h"
#include "uci/uci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{

    // data gen mode check
    if (argc >= 3 && strcmp(argv[1], "--gendata") == 0) {
        int num_games = atoi(argv[2]);
        int start_index = 0;
        const char* output_file = "training_data.bin";
        DatagenConfig config = datagen_default_config();

        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--start-index") == 0 && i + 1 < argc) {
                start_index = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--depth") == 0 && i + 1 < argc) {
                config.search_depth = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--temp") == 0 && i + 1 < argc) {
                config.temperature = strtod(argv[++i], NULL);
            } else if (strcmp(argv[i], "--max-plies") == 0 && i + 1 < argc) {
                config.max_plies = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--adjudicate-cp") == 0 && i + 1 < argc) {
                config.adjudicate_score_cp = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--adjudicate-plies") == 0 && i + 1 < argc) {
                config.adjudicate_plies = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
                output_file = argv[++i];
            } else {
                printf("Unknown or incomplete option: %s\n", argv[i]);
                printf("Usage: --gendata <num_games> [--start-index N] [--depth N] [--temp T] [--max-plies N] [--adjudicate-cp N] [--adjudicate-plies N] [--output FILE]\n");
                return 1;
            }
        }

        printf("Starting data generation for %d games...\n", num_games);
        generate_nnue_training_data_with_config(num_games, start_index, output_file, &config);
        printf("Generation complete. Exiting.\n");
        return 0;
    }

    uci_loop();
    return 0;
}
