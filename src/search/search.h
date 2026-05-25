#ifndef SEARCH_H
#define SEARCH_H
#include "engine/engine.h"
#include "movegen/move.h"

// Search tuning constants
#define MAX_KILLER_MOVES 2
#define MAX_PLY 64
#define MAX_LEGAL_MOVES 256
#define TT_MOVE_SCORE 2000000
#define GOOD_CAPTURE_BASE_SCORE 1200000
#define KILLER_1_SCORE 900000
#define KILLER_2_SCORE 800000
#define BAD_CAPTURE_BASE_SCORE 100000
#define HISTORY_MAX 200000
#define MATE_SCORE 200000000
#define MATE_THRESHOLD 199999000
#define MOVE_OVERHEAD_DEFAULT_MS 50

/**
 * @brief The main thread worker function that executes the search.
 *
 * Handles Iterative Deepening, time management tracking, and dynamic soft-limit
 * extensions (instability checks). Reporting is delegated to the engine.
 *
 * @param arg Pointer to a SearchThreadData struct containing the board state and limits.
 * @return NULL upon completion.
 */
void* search_worker(void* arg);

/**
 * @brief Handle a UCI "ponderhit" event by applying real-time controls.
 *
 * This updates the active search thread's soft/hard time limits so it can
 * continue searching with the proper time budget instead of pondering.
 */
void on_ponder_hit(void);

#endif // SEARCH_H