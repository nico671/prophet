#ifndef PROPHET_ENGINE_H
#define PROPHET_ENGINE_H

// Initializes global engine state (attack tables, zobrist keys, eval function helpers).
// Safe to call multiple times.
void initEngine(void);

#endif // PROPHET_ENGINE_H
