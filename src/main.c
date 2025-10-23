
#include <stdio.h>
#include "cboard.h"
#include "fen.h"
#include "generate_const_attacks.h"
#include "bitboard.h"
int main()
{
    Bitboard b = getKnightAttacks(D4);
    printBBoard(b);

    return 0;
}