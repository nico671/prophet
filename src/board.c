#include "board.h"

// rank 0 = white’s back rank, rank 1 = white pawns, 6 = black pawns, 7 = black back rank
void init_board(Board *b)
{
    // clear everything first
    memset(b, 0, sizeof *b);

    // white pawns
    for (int f = 0; f < 8; f++)
        bb_set(&b->white_pawns, square_index(1, f));

    // white rooks, knights, bishops, queen, king on rank 0
    bb_set(&b->white_rooks, square_index(0, 0));
    bb_set(&b->white_knights, square_index(0, 1));
    bb_set(&b->white_bishops, square_index(0, 2));
    bb_set(&b->white_queen, square_index(0, 3));
    bb_set(&b->white_king, square_index(0, 4));
    bb_set(&b->white_bishops, square_index(0, 5));
    bb_set(&b->white_knights, square_index(0, 6));
    bb_set(&b->white_rooks, square_index(0, 7));

    // mirror for black
    for (int f = 0; f < 8; f++)
        bb_set(&b->black_pawns, square_index(6, f));
    bb_set(&b->black_rooks, square_index(7, 0));
    bb_set(&b->black_knights, square_index(7, 1));
    bb_set(&b->black_bishops, square_index(7, 2));
    bb_set(&b->black_queen, square_index(7, 3));
    bb_set(&b->black_king, square_index(7, 4));
    bb_set(&b->black_bishops, square_index(7, 5));
    bb_set(&b->black_knights, square_index(7, 6));
    bb_set(&b->black_rooks, square_index(7, 7));

    // finally compute occupancies
    b->occupied_white = b->white_pawns | b->white_knights | b->white_bishops | b->white_rooks | b->white_queens | b->white_king;
    b->occupied_black = b->black_pawns | b->black_knights | b->black_bishops | b->black_rooks | b->black_queens | b->black_king;
    b->occupied_all = b->occupied_white | b->occupied_black;
}