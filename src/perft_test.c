#include "perft_test.h"
#include "sliding_attacks.h"

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
        printf("Move: from %d to %d, Nodes: %llu\n", move.from, move.to, nodes);

        // Unmake the move
        unmakeMove(board, move, undoInfo);
    }

    printf("Total nodes: %llu\n", totalNodes);
    return totalNodes;
}

int main()
{
    initSlidingAttacks();
    CBoard board;
    board = fenToCBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    for (int depth = 0; depth <= 10; depth++)
    {
        uint64_t nodes = perft(&board, depth);
        printf("Depth %d: %llu", depth, nodes);
        if (nodes == expected_nodes_initial_position[depth])
        {
            printf(" PASS\n");
        }
        else
        {
            printf(" FAIL, expected %llu\n", expected_nodes_initial_position[depth]);
            break;
        }
    }

    return 0;
}