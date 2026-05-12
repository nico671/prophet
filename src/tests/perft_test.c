#include "engine/engine.h"
#include "perft/perft.h"
#include "perft_test.h"
#include "search/search.h" //TODO: remove dependence on the string manipulation function here
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// REFERENCE: https://www.chessprogramming.org/Perft_Results
uint64_t expected_nodes_initial_position[] = {
    1ULL, // depth 0
    20ULL, // depth 1
    400ULL, // depth 2
    8902ULL, // depth 3
    197281ULL, // depth 4
    4865609ULL, // depth 5
    119060324ULL, // depth 6
    3195901860ULL, // depth 7
    84998978956ULL, // depth 8
    2439530234167ULL, // depth 9
    69352859712417ULL, // depth 10
};

uint64_t expected_nodes_kiwipete_position[] = {
    1ULL, // depth 0
    48ULL, // depth 1
    2039ULL, // depth 2
    97862ULL, // depth 3
    4085603ULL, // depth 4
    193690690ULL, // depth 5
    8031647685ULL, // depth 6
};

uint64_t expected_nodes_position_3[] = {
    1ULL, // depth 0
    14ULL, // depth 1
    191ULL, // depth 2
    2812ULL, // depth 3
    43238ULL, // depth 4
    674624ULL, // depth 5
    11030083ULL, // depth 6
    178633661ULL, // depth 7
    3009794393ULL, // depth 8
};
uint64_t expected_nodes_position_4[] = {
    1ULL,
    6ULL,
    264ULL,
    9467ULL,
    422333ULL,
    15833292ULL,
};

uint64_t expected_nodes_position_5[] = {
    1ULL,
    44ULL,
    1486ULL,
    62379ULL,
    2103487ULL,
    89941194ULL,
};

uint64_t expected_nodes_position_6[] = {
    1ULL,
    46ULL,
    2079ULL,
    89890ULL,
    3894594ULL,
    164075551ULL,
};

PerftTest test_suite[] = {
    {
        .name = "Initial Position",
        .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        .expected_nodes = expected_nodes_initial_position,
        .max_depth = 6, // 5 for speed, 6 is reasonable, 7+ takes too long
    },
    {
        .name = "Kiwipete Position",
        .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
        .expected_nodes = expected_nodes_kiwipete_position,
        .max_depth = 5, // 4 for speed, 5 is reasonable, 6+ takes too long
    },
    {
        .name = "Position 3",
        .fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        .expected_nodes = expected_nodes_position_3,
        .max_depth = 6,
    },
    {
        .name = "Position 4",
        .fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        .expected_nodes = expected_nodes_position_4,
        .max_depth = 5,
    },
    {
        .name = "Position 5",
        .fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        .expected_nodes = expected_nodes_position_5,
        .max_depth = 5, // 4 for speed, 5 is reasonable, 6+ takes too long
    },
    {
        .name = "Position 6",
        .fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        .expected_nodes = expected_nodes_position_6,
        .max_depth = 5,
    }
};

int main()
{
    engine_init();

    int num_tests = sizeof(test_suite) / sizeof(PerftTest);
    int total_passed = 0;
    int total_failed = 0;

    printf("Running %d perft test suites...\n\n", num_tests);

    for (int t = 0; t < num_tests; t++) {
        PerftTest test = test_suite[t];
        printf("=== %s ===\n", test.name);
        printf("FEN: %s\n", test.fen);
        CBoard board;
        bool parsedFen = fen_string_to_cboard(test.fen, &board);
        if (!parsedFen) {
            printf("Failed to parse FEN for test '%s'. Skipping this test.\n\n", test.name);
            total_failed += test.max_depth + 1; // Count all depths as failed for this test
            continue;
        }
        bool suite_passed = true;
        for (int depth = 1; depth <= test.max_depth; depth++) {
            clock_t start = clock();
            uint64_t nodes = perft(&board, depth);
            clock_t end = clock();
            double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

            uint64_t expected = test.expected_nodes[depth];

            printf("Depth %d: %llu (%.3fs, %.0f nodes/sec)",
                depth, nodes, elapsed, elapsed > 0 ? nodes / elapsed : 0);
            if (nodes == expected) {
                printf(" PASS\n");
                total_passed++;
            } else {
                printf(" FAIL (expected %llu)\n", expected);
                total_failed += test.max_depth - depth + 1;
                suite_passed = false;

                printf("\nRunning divide for depth %d:\n", depth);
                divide(&board, depth);
                break;
            }
        }

        printf("%s: %s\n\n", test.name, suite_passed ? "✓ PASSED" : "✗ FAILED");
    }

    printf("===========================================\n");
    printf("Results: %d passed, %d failed\n", total_passed, total_failed);

    return total_failed > 0 ? 1 : 0;
}