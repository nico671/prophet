#include <stddef.h>
#include <stdint.h>

#include "board/cboard.h"
#include "movegen/move.h"

typedef enum {
    TT_PV, // PV-nodes (Knuth's Type 1) are nodes that have a score that ends up
           // being inside the window. So if the bounds passed are [a,b], with
           // the score returned s, a<s<b. These nodes have all moves searched,
           // and the value returned is exact (i.e., not a bound), which
           // propagates up to the root along with a principal variation.
    TT_CUT, // known as fail-high nodes, are nodes in which a beta-cutoff was
            // performed. So with bounds [a,b], s>=b. A minimum of one move at a
            // Cut-node needs to be searched. The score returned is a lower bound
            // (might be greater) on the exact score of the node
    TT_ALL, // known as fail-low nodes, are nodes in which no move's score
            // exceeded alpha. With bounds [a,b], s<=a. Every move at an All-node
            // is searched, and the score returned is an upper bound, the exact
            // score might be less.
} TTBound;

typedef struct {
    uint64_t zobristKey; // To verify this entry matches the current board
    int score; // Evaluation score
    int depth; // Depth searched from this node
    TTBound bound; // The type of score
    Move bestMove; // The move that caused a cutoff or highest score
} TTEntry;

extern TTEntry* tt_table;
extern size_t tt_size; // Number of entries (must be a power of 2)
int extractPVLine(CBoard* board, Move* pvArray, int maxDepth);
void initTT(size_t megabytes);
void clearTT(void);
void storeTT(uint64_t key, int depth, int score, TTBound bound, Move bestMove);
TTEntry* probeTT(uint64_t key);