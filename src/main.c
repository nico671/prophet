
#include <stdio.h>
#include "cboard.h"
#include "fen.h"
#include "generate_const_attacks.h"
#include "bitboard.h"
int main()
{
    Bitboard b = square_mask(E4);
    printBBoard(b);
    return 0;
}