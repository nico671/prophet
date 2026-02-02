#ifndef PROPHET_ENGINE_H
#define PROPHET_ENGINE_H

// Initializes global engine state (attack tables, zobrist keys, etc).
// Safe to call multiple times.
void engine_init(void);

#endif // PROPHET_ENGINE_H
