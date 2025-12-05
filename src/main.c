
#include <stdio.h>
#include "cboard.h"
#include "fen.h"
#include "constant_attacks.h"
#include "bitboard.h"
#include "sliding_attacks.h"

int main()
{
    initSlidingAttacks(); // Initialize magic bitboard tables, must be run at the start
    CBoard board;
    const char *fenString = "r1bqkbnr/pppppppp/2n5/8/8/2N5/PPPPPPPP/R1BQKBNR w KQkq - 0 1";
    board = fenToCBoard((char *)fenString);
    if (!board.allPieces)
    {
        printf("Failed to load FEN: %s\n", fenString);
        return 1;
    }
    printBoard(&board);

    printBBoard(getBishopAttacks(E4, board.allPieces)); // Print bishop attacks from e4 (square 36)
    return 0;
}
