
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
    const char *fenString = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ";
    board = fenToCBoard((char *)fenString);

    printBoard(&board);
    MoveList test = {0};
    genAllPseudoLegalMoves(&board, &test);
    printf("%d", test.count);
    return 0;
}
