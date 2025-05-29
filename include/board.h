#include "bitboard.h"

typedef struct
{
    Bitboard white_pawns, white_knights, white_bishops;
    Bitboard white_rooks, white_queen, white_king;
    Bitboard black_pawns, black_knights, black_bishops;
    Bitboard black_rooks, black_queen, black_king;
    Bitboard occupied_white;
    Bitboard occupied_black;
    Bitboard occupied_all;
} Board;