#ifndef ENGINE_H
#define ENGINE_H
#include "board/cboard.h"
#include "movegen/move.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

// Global, thread-safe flag to interrupt the search
extern atomic_bool engine_stop_search;
extern atomic_bool engine_is_pondering;

/**
 * @brief Initializes the chess engine, setting up global state and data structures including sliding attack tables, Zobrist hashing keys, evaluation parameters, and the transposition table. This function is idempotent and can be safely called multiple times without adverse effects. It must be called before any search or move generation functions are used to ensure that all necessary data structures are properly initialized.
 *
 */
void init_engine(void);

#endif // ENGINE_H
