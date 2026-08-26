#ifndef PROPHET_TT_H
#define PROPHET_TT_H
#include "chess/board/cboard.h"
#include "chess/movegen/move.h"

#include <stddef.h>
#include <stdint.h>

#define TT_QSEARCH_DEPTH 0

/**
 * @brief Bound type stored in a transposition-table entry.
 *
 * PV is exact, CUT is a lower bound, and ALL is an upper bound.
 */
typedef enum {
    TT_PV,
    TT_CUT,
    TT_ALL,
} TTBound;

typedef struct {
    uint64_t zobrist_key; // To verify this entry matches the current board
    int score; // Evaluation score
    int depth; // Depth searched from this node
    TTBound bound; // The type of score
    Move best_move; // The move that caused a cutoff or highest score
} TTEntry;

extern TTEntry* tt_table;
extern size_t tt_size; // Number of entries (must be a power of 2)

/** @brief Reconstructs a principal variation from transposition-table moves. */
int extract_pv_line(CBoard* board, Move* pv_move_list, int max_depth);

/** @brief Allocates and clears a table of up to @p megabytes. */
void init_tt(size_t megabytes);

/** @brief Invalidates every transposition-table entry. */
void clear_tt(void);

/** @brief Stores a search result, replacing the current slot when appropriate.
 */
void store_tt(uint64_t key, int depth, int score, TTBound bound, Move best_move);

/** @brief Returns the matching entry for @p key, or NULL. */
TTEntry* probe_tt(uint64_t key);

/** @brief Releases the allocated table and resets its globals. */
void free_tt(void);

#endif // PROPHET_TT_H
