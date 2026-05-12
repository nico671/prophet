#include "perft/perft.h"

#include <stdio.h>

#include "movegen/move.h"
#include "movegen/move_make.h"
#include "movegen/movegen.h"

uint64_t perft(CBoard* board, int depth)
{
    if (depth == 0) {
        return 1;
    }

    MoveList moveList;
    init_move_list(&moveList);
    generate_legal_moves(board, &moveList);
    if (depth == 1) {
        return (uint64_t)moveList.count;
    }

    uint64_t nodes = 0;
    for (int i = 0; i < moveList.count; i++) {
        Move move = moveList.moves[i];

        // Make the move
        UndoInfo undoInfo = make_move(board, move);

        // Recurse
        nodes += perft(board, depth - 1);

        // Unmake the move
        unmake_move(board, move, undoInfo);
    }

    return nodes;
}

uint64_t divide(CBoard* board, int depth)
{
    MoveList moveList;
    init_move_list(&moveList);
    generate_legal_moves(board, &moveList);

    uint64_t totalNodes = 0;
    for (int i = 0; i < moveList.count; i++) {
        Move move = moveList.moves[i];

        // Make the move
        UndoInfo undoInfo = make_move(board, move);

        // Recurse
        uint64_t nodes = perft(board, depth - 1);
        totalNodes += nodes;

        // Print the move and its node count
        char* moveStr = moveToStringCoordinate(move);
        printf("%s: %llu\n", moveStr, nodes);

        // Unmake the move
        unmake_move(board, move, undoInfo);
    }

    printf("Total nodes: %llu\n", totalNodes);
    return totalNodes;
}
