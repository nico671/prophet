#ifndef PROPHET_BINPACK_WRITER_H
#define PROPHET_BINPACK_WRITER_H

#include "chess/board/cboard.h"
#include "chess/movegen/move.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void* file;
    unsigned char* buffer;
    size_t used;
    size_t records;
} BinpackWriter;

bool binpack_open(BinpackWriter* writer, const char* path);
bool binpack_append(BinpackWriter* writer, const CBoard* board, Move move, int score, int ply,
                    double result);
bool binpack_close(BinpackWriter* writer);

#endif // PROPHET_BINPACK_WRITER_H
