
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
    MoveList test = {0};
    genAllPseudoLegalMoves(&board, &test);
    printf("%d", test.count);
    return 0;
}
