#ifndef SEARCH_H
#define SEARCH_H
#include "engine/engine.h"
#include "movegen/move.h"
#include "uci/uci.h"

typedef struct SearchLimits {
    bool ponder;
    bool infinite_search;
    int time_for_white_ms;
    int time_for_black_ms;
    int increment_for_white_ms;
    int increment_for_black_ms;
    int moves_until_next_time_control;
    int depth_limit;
    int node_limit;
    int search_for_mate_in_n_moves;
    int time_limit_ms;
    MoveList search_moves;
} SearchLimits;

// The payload we send to the search thread
typedef struct {
    CBoard board; // A COPY of the board, safe from UCI mutations
    SearchLimits search_limits; // The parsed go parameters
} SearchThreadData;

typedef struct {
    Move move;
    int score;
} ScoredMove;

// killer moves storage
#define MAX_KILLER_MOVES 2
#define MAX_PLY 64
extern Move killer_moves_list[MAX_PLY]
                             [MAX_KILLER_MOVES]; // [depth][idx] where 0 is newest
                                                 // killer, 1 is previous killer

extern int history_heuristic[2][64][64]; // [color][from][to]

void* search_worker(void* arg);

void search_on_go_command(UCIState* state, SearchLimits search_limits);

void score_moves(CBoard* board, MoveList* move_list, ScoredMove* scored_moves,
    Move tt_move, int ply);
int quiescence(CBoard* node, int alpha, int beta, int ply);
int negamax(CBoard* node, int depth, int alpha, int beta, Color color, int ply);
void pick_next_best_move(ScoredMove* scored_moves, int start, int count);
void clear_search_heuristics(void);
void on_ponder_hit(void);
#endif // SEARCH_H