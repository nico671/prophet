
#include <stdio.h>
#include "cboard.h"
#include "fen.h"
#include "generate_const_attacks.h"
#include "bitboard.h"
#include "sliding_attacks.h"

int main()
{
    computeBishopOccupancyMaps();
    return 0;
}