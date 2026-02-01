
#include <stdio.h>
#include "board/cboard.h"
#include "attacks/constant_attacks.h"
#include "core/bitboard.h"
#include "attacks/sliding_attacks.h"
#include "movegen/movegen.h"
#include "movegen/move_make.h"
#include "hcevaluation/hceval.h"
int main()
{
    initSlidingAttacks();

    // Test zobrist hashing with make/unmake
    printf("=== Testing Zobrist Hashing ===\n\n");

    CBoard board = fenToCBoard((char *)"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    printf("Initial position:\n");
    printBoard(&board);
    int position_score = evaluateBoard(&board);
    printf("Position score: %d\n", position_score);

    board = fenToCBoard((char *)"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");

    printf("Position 4:\n");
    printBoard(&board);
    position_score = evaluateBoard(&board);
    printf("Position score: %d\n", position_score);
    return 0;
}
