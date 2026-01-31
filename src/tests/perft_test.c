#include "perft_test.h"
#include <string.h>

typedef struct
{
    const char *name;
    const char *fen;
    uint64_t *expected_nodes;
    int max_depth;
} PerftTest;

uint64_t expected_nodes_initial_position[] = {
    1ULL,
    20ULL,
    400ULL,
    8902ULL,
    197281ULL,
    4865609ULL,
    119060324ULL,
    3195901860ULL,
    84998978956ULL,
    2439530234167ULL,
    69352859712417ULL};

uint64_t expected_nodes_kiwipete_position[] = {
    1ULL,
    48ULL,
    2039ULL,
    97862ULL,
    4085603ULL,
    193690690ULL,
    8031647685ULL,
};
PerftTest test_suite[] = {
    {
        .name = "Initial Position",
        .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        .expected_nodes = expected_nodes_initial_position,
        .max_depth = 5, // 5 for speed, 6 is reasonable, 7+ takes too long
    },
    // {
    //     .name = "Kiwipete Position",
    //     .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
    //     .expected_nodes = expected_nodes_kiwipete_position,
    //     .max_depth = 5,
    // }
};
uint64_t perft(CBoard *board, int depth)
{
    if (depth == 0)
        return 1;

    MoveList moveList;
    moveList.count = 0;
    moveList = generateLegalMoves(board);

    uint64_t nodes = 0;
    for (int i = 0; i < moveList.count; i++)
    {
        Move move = moveList.moves[i];

        // Make the move
        UndoInfo undoInfo;
        undoInfo = makeMove(board, move);

        // Recurse
        nodes += perft(board, depth - 1);

        // Unmake the move
        unmakeMove(board, move, undoInfo);
    }

    return nodes;
}

char *squareToString(Square sq)
{
    static char str[3];
    char files[] = "abcdefgh";
    char ranks[] = "12345678";
    str[0] = files[sq % 8];
    str[1] = ranks[sq / 8];
    str[2] = '\0';
    return str;
}

uint64_t divide(CBoard *board, int depth)
{
    MoveList moveList;
    moveList.count = 0;
    moveList = generateLegalMoves(board);

    uint64_t totalNodes = 0;
    for (int i = 0; i < moveList.count; i++)
    {
        Move move = moveList.moves[i];

        // Make the move
        UndoInfo undoInfo;
        undoInfo = makeMove(board, move);

        // Recurse
        uint64_t nodes = perft(board, depth - 1);
        totalNodes += nodes;

        // Print the move and its node count
        char *fromStr = squareToString(move.from);
        char *toStr = squareToString(move.to);
        printf("Move: %s%s, Nodes: %llu\n", fromStr, toStr, nodes);
        // No need to free fromStr and toStr since they point to static memory
        //
        // printf("Move: from %d to %d, Nodes: %llu\n", move.from, move.to, nodes);

        // Unmake the move
        unmakeMove(board, move, undoInfo);
    }

    printf("Total nodes: %llu\n", totalNodes);
    return totalNodes;
}

#include "perft_test.h"
#include <string.h>
#include <time.h>

// ...existing code...

int main()
{
    initSlidingAttacks();

    int num_tests = sizeof(test_suite) / sizeof(PerftTest);
    int total_passed = 0;
    int total_failed = 0;

    printf("Running %d perft test suites...\n\n", num_tests);

    for (int t = 0; t < num_tests; t++)
    {
        PerftTest test = test_suite[t];
        printf("=== %s ===\n", test.name);
        printf("FEN: %s\n", test.fen);

        CBoard board = fenToCBoard(test.fen);
        bool suite_passed = true;

        for (int depth = 0; depth <= test.max_depth; depth++)
        {
            clock_t start = clock();
            uint64_t nodes = perft(&board, depth);
            clock_t end = clock();
            double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

            uint64_t expected = test.expected_nodes[depth];

            printf("Depth %d: %llu (%.3fs, %.0f nodes/sec)",
                   depth, nodes, elapsed, elapsed > 0 ? nodes / elapsed : 0);
            if (nodes == expected)
            {
                printf(" PASS\n");
                total_passed++;
            }
            else
            {
                printf(" FAIL (expected %llu)\n", expected);
                total_failed++;
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