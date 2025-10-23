
#include <stdio.h>
#include "cboard.h"
#include "fen.h"
#include "generate_const_attacks.h"
#include "bitboard.h"
int main()
{
    for (int sq = 0; sq < 64; sq++)
    {
        Bitboard b = getBlackPawnAttacks(sq);
        printf("Black pawn attacks from square %d:\n", sq);
        printBBoard(b);
    }

    return 0;
}