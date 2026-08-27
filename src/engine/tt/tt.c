#include "tt.h"

#include "chess/board/cboard.h"
#include "chess/movegen/move.h"
#include "chess/movegen/move_make.h"
#include "chess/movegen/movegen.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

TTEntry* tt_table = NULL;
size_t tt_size    = 0;

static bool is_move_legal_in_position(CBoard* board, Move move)
{
    MoveList move_list;
    init_move_list(&move_list);
    generate_legal_moves(board, &move_list);

    for (int i = 0; i < move_list.count; i++) {
        if (move_list.moves[i] == move) {
            return true;
        }
    }

    return false;
}

static size_t floor_pow2(size_t x)
{
    if (x == 0) {
        return 0;
    }
    size_t p = 1;
    while ((p << 1) > p && (p << 1) <= x) // overflow-safe
    {
        p <<= 1;
    }
    return p;
}

static bool has_stored_move(Move move)
{
    return move_get_from_square(move) != NO_SQUARE;
}

void init_tt(size_t megabytes)
{
    if (tt_table != NULL) {
        free(tt_table);
        tt_table = NULL;
        tt_size  = 0;
    }

    size_t entries = (megabytes * 1024 * 1024) / sizeof(TTEntry);
    tt_size        = floor_pow2(entries);
    if (tt_size == 0) {
        fprintf(stderr, "Transposition table size must be greater than 0\n");
        exit(1);
    }
    tt_table = (TTEntry*)malloc(tt_size * sizeof(TTEntry));
    if (tt_table == NULL) {
        fprintf(stderr, "Failed to allocate transposition table\n");
        exit(1);
    }
    clear_tt();
}

void clear_tt(void)
{
    for (size_t i = 0; i < tt_size; i++) {
        tt_table[i].zobrist_key = 0;
        tt_table[i].score       = 0;
        tt_table[i].depth       = -1;
        tt_table[i].bound       = TT_ALL;
        tt_table[i].best_move   = create_move(NO_SQUARE, NO_SQUARE, NORMAL, NO_PIECE);
    }
}

void store_tt(uint64_t key, int depth, int score, TTBound bound, Move best_move)
{
    if (key == 0 || tt_size == 0 || tt_table == NULL) {
        return;
    }

    size_t index = key & (tt_size - 1); // Equivalent to key % tt_size when
    // tt_size is a power of 2
    TTEntry* entry        = &tt_table[index];

    bool key_matches      = entry->zobrist_key == key;
    bool empty_slot       = entry->zobrist_key == 0;
    bool old_has_move     = has_stored_move(entry->best_move);
    bool new_has_move     = has_stored_move(best_move);
    bool same_depth       = depth == entry->depth;
    bool exact_tie        = same_depth && bound == TT_PV && entry->bound != TT_PV;
    bool replace_same_key = key_matches && depth > entry->depth;
    replace_same_key      = replace_same_key
        || (key_matches && same_depth && (bound == TT_PV || entry->bound != TT_PV));
    bool replace_collision = !key_matches && (depth > entry->depth || exact_tie);

    if (empty_slot || replace_same_key || replace_collision) {
        Move stored_move = new_has_move ? best_move : entry->best_move;
        *entry           = (TTEntry) {
            .zobrist_key = key,
            .score       = score,
            .depth       = depth,
            .bound       = bound,
            .best_move   = stored_move,
        };
    } else if (key_matches && !old_has_move && new_has_move) {
        // Preserve a deeper score and bound while filling its missing move.
        entry->best_move = best_move;
    }
}

TTEntry* probe_tt(uint64_t key)
{
    if (key == 0 || tt_size == 0 || tt_table == NULL) {
        return NULL; // TT not initialized or invalid key
    }
    size_t index = key & (tt_size - 1); // Equivalent to key % tt_size when
    // tt_size is a power of 2
    TTEntry* entry = &tt_table[index];
    if (entry->zobrist_key == key) {
        return entry;
    }
    return NULL;
}

int extract_pv_line(CBoard* board, Move* pv_move_list, int max_depth)
{
    int count = 0;
    UndoInfo undo_stack[256];

    while (count < max_depth) {
        TTEntry* entry = probe_tt(board->zobrist_key);

        // Stop if no TT entry, or if the TT entry doesn't have a
        // valid move
        if (!entry || entry->zobrist_key != board->zobrist_key
            || move_get_from_square(entry->best_move) == NO_SQUARE) {
            break;
        }

        Move pv_move = entry->best_move;

        // TT entries can be stale/colliding so check if the move is
        // legal
        if (!is_move_legal_in_position(board, pv_move)) {
            break;
        }

        pv_move_list[count] = pv_move;

        // Make the move on the board to update the Zobrist key for
        // the next lookup
        undo_stack[count] = make_move(board, pv_move);
        count++;
    }

    // Unmake all moves to restore the root board state
    for (int i = count - 1; i >= 0; i--) {
        unmake_move(board, pv_move_list[i], undo_stack[i]);
    }

    return count;
}

void free_tt(void)
{
    if (tt_table != NULL) {
        free(tt_table);
        tt_table = NULL;
        tt_size  = 0;
    }
}
