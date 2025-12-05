
#include <stdio.h>
#include "cboard.h"
#include "fen.h"
#include "constant_attacks.h"
#include "bitboard.h"
#include "sliding_attacks.h"

int main()
{
    initSlidingAttacks();

    CBoard board;
    const char *fenString = "r1bqkbnr/pppppppp/2n5/8/8/2N5/PPPPPPPP/R1BQKBNR w KQkq - 0 1";
    board = fenToCBoard((char *)fenString);

    printf("Testing generateBishopAttacks directly for c1:\n");
    printf("With no blockers:\n");
    printBBoard(generateBishopAttacks(C1, 0ULL));

    printf("\nWith board blockers:\n");
    printBBoard(generateBishopAttacks(C1, board.allPieces));

    printf("\nDebug: Testing c1 bishop\n");
    printf("Raw mask for c1:\n");
    // Print the mask used for c1 bishop attacks
    extern const Bitboard bishop_occupancy_maps[];
    printBBoard(bishop_occupancy_maps[C1]);

    printf("\nBlockers: 0x%llx\n", board.allPieces);
    printf("Relevant blockers should be on diagonals only\n");

    // Try manual diagonal calculation to compare
    Bitboard manualAttacks = 0ULL;
    // Set bits on diagonals from c1: a3,b2,d2,e3,f4,g5,h6
    manualAttacks |= (1ULL << A3) | (1ULL << B2) | (1ULL << D2) | (1ULL << E3) | (1ULL << F4) | (1ULL << G5) | (1ULL << H6);
    printf("\nExpected diagonal squares from c1:\n");
    printBBoard(manualAttacks);
    // Test bishop on empty board first
    printf("Bishop attacks from c1 (empty board):\n");
    printBBoard(getBishopAttacks(C1, 0ULL));

    printf("\nBishop attacks from c1 (with pieces):\n");
    printBBoard(getBishopAttacks(C1, board.allPieces));
    return 0;
}
