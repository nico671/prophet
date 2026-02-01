
#include <stdio.h>
#include "board/cboard.h"
#include "attacks/constant_attacks.h"
#include "core/bitboard.h"
#include "attacks/sliding_attacks.h"
#include "movegen/movegen.h"
#include "movegen/move_make.h"
#include "search/search.h"
#include "hcevaluation/hceval.h"
int main()
{
    initSlidingAttacks();

    CBoard board = fenToCBoard((char *)"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    printf("Initial position:\n");
    printBoard(&board);
    int position_score = evaluateBoard(&board);
    printf("Position score: %d\n", position_score);
    int search_result = negamax(&board, 6, -1000000, 1000000, WHITE);
    printf("Search result: %d\n", search_result);
    board = fenToCBoard((char *)"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");

    printf("Position 4:\n");
    printBoard(&board);
    position_score = evaluateBoard(&board);
    printf("Position score: %d\n", position_score);
    search_result = negamax(&board, 6, -1000000, 1000000, WHITE);
    printf("Search result: %d\n", search_result);

    return 0;
}
