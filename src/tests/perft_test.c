// #include "engine/engine.h"
// #include "perft/perft.h"
// #include "perft_test.h"
// #include "search/search.h" //TODO: remove dependence on the string manipulation function here
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <time.h>

// int main()
// {
//     engine_init();

//     int num_tests = sizeof(test_suite) / sizeof(PerftTest);
//     int total_passed = 0;
//     int total_failed = 0;

//     printf("Running %d perft test suites...\n\n", num_tests);

//     for (int t = 0; t < num_tests; t++) {
//         PerftTest test = test_suite[t];
//         printf("=== %s ===\n", test.name);
//         printf("FEN: %s\n", test.fen);
//         CBoard board;
//         bool parsedFen = fen_string_to_cboard(test.fen, &board);
//         if (!parsedFen) {
//             printf("Failed to parse FEN for test '%s'. Skipping this test.\n\n", test.name);
//             total_failed += test.max_depth + 1; // Count all depths as failed for this test
//             continue;
//         }
//         bool suite_passed = true;
//         for (int depth = 1; depth <= test.max_depth; depth++) {
//             clock_t start = clock();
//             uint64_t nodes = perft(&board, depth);
//             clock_t end = clock();
//             double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

//             uint64_t expected = test.expected_nodes[depth];

//             printf("Depth %d: %llu (%.3fs, %.0f nodes/sec)",
//                 depth, nodes, elapsed, elapsed > 0 ? nodes / elapsed : 0);
//             if (nodes == expected) {
//                 printf(" PASS\n");
//                 total_passed++;
//             } else {
//                 printf(" FAIL (expected %llu)\n", expected);
//                 total_failed += test.max_depth - depth + 1;
//                 suite_passed = false;

//                 printf("\nRunning divide for depth %d:\n", depth);
//                 divide(&board, depth);
//                 break;
//             }
//         }

//         printf("%s: %s\n\n", test.name, suite_passed ? "✓ PASSED" : "✗ FAILED");
//     }

//     printf("===========================================\n");
//     printf("Results: %d passed, %d failed\n", total_passed, total_failed);

//     return total_failed > 0 ? 1 : 0;
// }