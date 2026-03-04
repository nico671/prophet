#include "tt.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "board/cboard.h"
#include "board/undo.h"
#include "movegen/move.h"
#include "movegen/move_make.h"
TTEntry *tt_table = NULL;
size_t tt_size = 0;

static size_t floor_pow2(size_t x)
{
    if (x == 0)
        return 0;
    size_t p = 1;
    while ((p << 1) > p && (p << 1) <= x) // overflow-safe
    {
        p <<= 1;
    }
    return p;
}

void initTT(size_t megabytes)
{
    size_t entries = (megabytes * 1024 * 1024) / sizeof(TTEntry);
    tt_size = floor_pow2(entries);
    if (tt_size == 0)
    {
        fprintf(stderr, "Transposition table size must be greater than 0\n");
        exit(1);
    }
    tt_table = (TTEntry *)malloc(tt_size * sizeof(TTEntry));
    if (tt_table == NULL)
    {
        fprintf(stderr, "Failed to allocate transposition table\n");
        exit(1);
    }
    clearTT();
}

void clearTT()
{
    for (size_t i = 0; i < tt_size; i++)
    {
        tt_table[i].zobristKey = 0;
        tt_table[i].score = 0;
        tt_table[i].depth = -1;
        tt_table[i].bound = TT_ALL;
        tt_table[i].bestMove = (Move){.from = NO_SQUARE, .to = NO_SQUARE, .flag = 0};
    }
}

void storeTT(uint64_t key, int depth, int score, TTBound bound, Move bestMove)
{
    size_t index = key & (tt_size - 1); // Equivalent to key % tt_size when tt_size is a power of 2
    TTEntry *entry = &tt_table[index];

    // Always replace if the new entry has greater depth or is an exact score
    if (depth > entry->depth || bound == TT_PV)
    {
        entry->zobristKey = key;
        entry->score = score;
        entry->depth = depth;
        entry->bound = bound;
        entry->bestMove = bestMove;
    }
}

TTEntry *probeTT(uint64_t key)
{
    size_t index = key & (tt_size - 1); // Equivalent to key % tt_size when tt_size is a power of 2
    TTEntry *entry = &tt_table[index];
    if (entry->zobristKey == key)
    {
        return entry;
    }
    return NULL;
}

int extractPVLine(CBoard *board, Move *pvArray, int maxDepth)
{
    int count = 0;
    UndoInfo undoStack[256];

    while (count < maxDepth)
    {
        TTEntry *entry = probeTT(board->zobristKey);

        // Stop if no TT entry, or if the TT entry doesn't have a valid move
        if (!entry || entry->zobristKey != board->zobristKey || entry->bestMove.from == NO_SQUARE)
        {
            break;
        }

        Move pvMove = entry->bestMove;
        pvArray[count] = pvMove;

        // Make the move on the board to update the Zobrist key for the next lookup
        undoStack[count] = makeMove(board, pvMove);
        count++;
    }

    // Unmake all moves to restore the root board state!
    for (int i = count - 1; i >= 0; i--)
    {
        unmakeMove(board, pvArray[i], undoStack[i]);
    }

    return count;
}