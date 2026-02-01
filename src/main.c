
#include <stdio.h>
#include "board/cboard.h"
#include "attacks/constant_attacks.h"
#include "core/bitboard.h"
#include "attacks/sliding_attacks.h"
#include "movegen/movegen.h"
#include "movegen/move_make.h"

int main()
{
    initSlidingAttacks();

    // Test zobrist hashing with make/unmake
    printf("=== Testing Zobrist Hashing ===\n\n");

    CBoard board = fenToCBoard((char *)"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    printf("Initial position:\n");
    printBoard(&board);
    uint64_t initialKey = board.zobristKey;
    printf("Initial zobrist key: 0x%016llx\n\n", initialKey);

    // Generate moves
    MoveList moveList = {0};
    genAllPseudoLegalMoves(&board, &moveList);
    printf("Generated %d moves\n\n", moveList.count);

    // Test make/unmake with first few moves
    for (int i = 0; i < 5 && i < moveList.count; i++)
    {
        Move move = moveList.moves[i];
        printf("Move %d: from %d to %d\n", i + 1, FROM_SQ(move), TO_SQ(move));

        // Make move
        UndoInfo undoInfo = makeMove(&board, move);
        uint64_t afterMoveKey = board.zobristKey;
        printf("  After move key:  0x%016llx\n", afterMoveKey);

        // Recompute from scratch
        uint64_t savedKey = board.zobristKey;
        computeZobristKey(&board);
        uint64_t recomputedKey = board.zobristKey;
        printf("  Recomputed key:  0x%016llx\n", recomputedKey);

        if (savedKey == recomputedKey)
        {
            printf("  ✓ Incremental update matches recomputation\n");
        }
        else
        {
            printf("  ✗ ERROR: Key mismatch!\n");
        }

        // Unmake move
        unmakeMove(&board, move, undoInfo);
        uint64_t afterUnmakeKey = board.zobristKey;
        printf("  After unmake key: 0x%016llx\n", afterUnmakeKey);

        if (afterUnmakeKey == initialKey)
        {
            printf("  ✓ Unmake restored original key\n");
        }
        else
        {
            printf("  ✗ ERROR: Unmake failed to restore key!\n");
        }
        printf("\n");
    }

    printf("All zobrist hashing tests completed!\n");
    return 0;
}
