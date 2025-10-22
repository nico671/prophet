
#include <stdio.h>
#include "board.h"
#include "fen.h"
int main()
{
    char someFen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    CBoard board = fenToCBoard(someFen);
    printBoard(&board);
    char *retfen = CBoardToFen(&board);
    printf("FEN: %s\n", retfen);
    free(retfen);
    return 0;
}