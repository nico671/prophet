
#include <stdio.h>
#include "board/cboard.h"
#include "engine/engine.h"
#include "search/search.h"
#include "hcevaluation/hceval.h"
int main()
{
    engine_init();

    CBoard board = fenToCBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    printf("Initial position:\n");
    printBoard(&board);
    int position_score = evaluateBoard(&board);
    printf("Position score: %d\n", position_score);
    int search_result = negamax(&board, 6, -1000000, 1000000, WHITE);
    printf("Search result: %d\n", search_result);
    board = fenToCBoard("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");

    printf("Position 4:\n");
    printBoard(&board);
    position_score = evaluateBoard(&board);
    printf("Position score: %d\n", position_score);
    search_result = negamax(&board, 6, -1000000, 1000000, WHITE);
    printf("Search result: %d\n", search_result);

    return 0;
}
