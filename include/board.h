#include <stdbool.h>
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
    bool white_to_move;
    uint8_t castling_rights;   // first 4 bits: white kingside, white queenside, black kingside, black queenside
    uint8_t en_passant_square; // 0-63 for a square, 64 if no en passant
} Board;