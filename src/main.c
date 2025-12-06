
#include <stdio.h>
#include "cboard.h"
#include "fen.h"
#include "constant_attacks.h"
#include "bitboard.h"
#include "sliding_attacks.h"
#include "movegen.h"
int main()
{
    initSlidingAttacks();

    CBoard board;
    const char *fenString = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    board = fenToCBoard((char *)fenString);

    printBoard(&board);
    // CBoard board1 = gen_pawn_pushes(&board);
    // printBoard(&board1);
    return 0;
}
