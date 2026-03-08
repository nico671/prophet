#include "tt.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "board/cboard.h"
#include "movegen/move.h"
#include "movegen/move_make.h"
#include "movegen/movegen.h"

TTEntry *tt_table = NULL;
size_t tt_size = 0;

static bool isMoveLegalInPosition(CBoard *board, Move move)
{
    MoveList moveList;
    initMoveList(&moveList);
    generateLegalMoves(board, &moveList);

    for (int i = 0; i < moveList.count; i++)
    {
        if (moveList.moves[i] == move)
        {
            return true;
        }
    }

    return false;
}

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
        tt_table[i].bestMove = createMove(NO_SQUARE, NO_SQUARE, NORMAL, NO_PIECE);
    }
}

void storeTT(uint64_t key, int depth, int score, TTBound bound, Move bestMove)
{
    if (key == 0 || tt_size == 0 || tt_table == NULL)
    {
        return;
    }

    size_t index = key & (tt_size - 1); // Equivalent to key % tt_size when tt_size is a power of 2
    TTEntry *entry = &tt_table[index];

    bool sameKey = (entry->zobristKey == key);
    bool emptySlot = (entry->zobristKey == 0);

    // Replace if slot is empty, if we're refreshing same position, or if this entry is more valuable
    if (emptySlot || sameKey || depth > entry->depth || bound == TT_PV)
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
    if (key == 0 || tt_size == 0 || tt_table == NULL)
    {
        return NULL; // TT not initialized or invalid key
    }
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
        if (!entry || entry->zobristKey != board->zobristKey || getFromSquare(entry->bestMove) == NO_SQUARE)
        {
            break;
        }

        Move pvMove = entry->bestMove;

        // Defensive safety: TT entries can be stale/colliding; only follow legal moves.
        if (!isMoveLegalInPosition(board, pvMove))
        {
            break;
        }

        pvArray[count] = pvMove;

        // Make the move on the board to update the Zobrist key for the next lookup
        undoStack[count] = makeMove(board, pvMove);
        count++;
    }

    // Unmake all moves to restore the root board state
    for (int i = count - 1; i >= 0; i--)
    {
        unmakeMove(board, pvArray[i], undoStack[i]);
    }

    return count;
}