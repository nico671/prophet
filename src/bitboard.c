#include "bitboard.h"

// rank 0 = bits 0–7; rank 7 = bits 56–63
const Bitboard RANK_MASK[8] = {
    0x00000000000000FFULL,
    0x000000000000FF00ULL,
    0x0000000000FF0000ULL,
    0x00000000FF000000ULL,
    0x000000FF00000000ULL,
    0x0000FF0000000000ULL,
    0x00FF000000000000ULL,
    0xFF00000000000000ULL};

// file a = bits 0,8,16…56; file h = bits 7,15…63
const Bitboard FILE_MASK[8] = {
    0x0101010101010101ULL,
    0x0202020202020202ULL,
    0x0404040404040404ULL,
    0x0808080808080808ULL,
    0x1010101010101010ULL,
    0x2020202020202020ULL,
    0x4040404040404040ULL,
    0x8080808080808080ULL};

void print_bitboard(Bitboard b)
{
    for (int r = 7; r >= 0; r--)
    {
        for (int f = 0; f < 8; f++)
        {
            int sq = square_index(r, f);
            putchar(bb_test(b, sq) ? '1' : '.');
            putchar(' ');
        }
        putchar('\n');
    }
    puts("a b c d e f g h");
}