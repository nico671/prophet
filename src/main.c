
#include <stdio.h>
#include "board/cboard.h"
#include "board/fen.h"
#include "attacks/constant_attacks.h"
#include "core/bitboard.h"
#include "attacks/sliding_attacks.h"
#include "movegen/movegen.h"
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
