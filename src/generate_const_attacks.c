#include "generate_const_attacks.h"

// precomputed knight attack bitboards for each square, indexing same as Square enum
const Bitboard knight_attacks[64] = {
    0x0000000000020400ULL,
    0x0000000000050800ULL,
    0x00000000000a1100ULL,
    0x0000000000142200ULL,
    0x0000000000284400ULL,
    0x0000000000508800ULL,
    0x0000000000a01000ULL,
    0x0000000000402000ULL,
    0x0000000002040004ULL,
    0x0000000005080008ULL,
    0x000000000a110011ULL,
    0x0000000014220022ULL,
    0x0000000028440044ULL,
    0x0000000050880088ULL,
    0x00000000a0100010ULL,
    0x0000000040200020ULL,
    0x0000000204000402ULL,
    0x0000000508000805ULL,
    0x0000000a1100110aULL,
    0x0000001422002214ULL,
    0x0000002844004428ULL,
    0x0000005088008850ULL,
    0x000000a0100010a0ULL,
    0x0000004020002040ULL,
    0x0000020400040200ULL,
    0x0000050800080500ULL,
    0x00000a1100110a00ULL,
    0x0000142200221400ULL,
    0x0000284400442800ULL,
    0x0000508800885000ULL,
    0x0000a0100010a000ULL,
    0x0000402000204000ULL,
    0x0002040004020000ULL,
    0x0005080008050000ULL,
    0x000a1100110a0000ULL,
    0x0014220022140000ULL,
    0x0028440044280000ULL,
    0x0050880088500000ULL,
    0x00a0100010a00000ULL,
    0x0040200020400000ULL,
    0x0204000402000000ULL,
    0x0508000805000000ULL,
    0x0a1100110a000000ULL,
    0x1422002214000000ULL,
    0x2844004428000000ULL,
    0x5088008850000000ULL,
    0xa0100010a0000000ULL,
    0x4020002040000000ULL,
    0x0400040200000000ULL,
    0x0800080500000000ULL,
    0x1100110a00000000ULL,
    0x2200221400000000ULL,
    0x4400442800000000ULL,
    0x8800885000000000ULL,
    0x100010a000000000ULL,
    0x2000204000000000ULL,
    0x0004020000000000ULL,
    0x0008050000000000ULL,
    0x00110a0000000000ULL,
    0x0022140000000000ULL,
    0x0044280000000000ULL,
    0x0088500000000000ULL,
    0x0010a00000000000ULL,
    0x0020400000000000ULL,
};

// the following function was used to generate the above constant array
// for knight attacks
// uncomment and run separately to regenerate if needed
// Bitboard knight_attacks[64];
// void initKnightAttacks()
// {
//     for (int sq = 0; sq < 64; sq++)
//     {
//         Bitboard attacks = 0;
//         int rank = sq / 8;
//         int file = sq % 8;

//         int knight_moves[8][2] = {
//             {2, 1}, {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2}, {1, -2}, {2, -1}};

//         for (int i = 0; i < 8; i++)
//         {
//             int new_rank = rank + knight_moves[i][0];
//             int new_file = file + knight_moves[i][1];

//             if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8)
//             {
//                 attacks |= ((Bitboard)1 << (new_rank * 8 + new_file));
//             }
//         }
//         knight_attacks[sq] = attacks;
//     }
//     // print knight attacks for storage as constant array
//     printf("const Bitboard knight_attacks[64] = {\n");
//     for (int i = 0; i < 64; i++)
//     {
//         printf("    0x%016llxULL,%s\n", knight_attacks[i], (i % 8 == 7) ? "" : " ");
//     }
//     printf("};\n");
// }

Bitboard getKnightAttacks(int square)
{
    return knight_attacks[square];
}

// this function generates and prints the king attack bitboards for each square, see the results below
// Bitboard king_attacks[64];

// void initKingAttacks()
// {
//     for (int sq = 0; sq < 64; sq++)
//     {
//         Bitboard attacks = 0;
//         int rank = sq / 8;
//         int file = sq % 8;

//         int king_moves[8][2] = {
//             {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};

//         for (int i = 0; i < 8; i++)
//         {
//             int new_rank = rank + king_moves[i][0];
//             int new_file = file + king_moves[i][1];

//             if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8)
//             {
//                 attacks |= ((Bitboard)1 << (new_rank * 8 + new_file));
//             }
//         }
//         king_attacks[sq] = attacks;
//     }
//     printf("const Bitboard king_attacks[64] = {\n");
//     for (int i = 0; i < 64; i++)
//     {
//         printf("    0x%016llxULL,%s\n", king_attacks[i], (i % 8 == 7) ? "" : " ");
//     }
//     printf("};\n");
// }
const Bitboard king_attacks[64] = {
    0x0000000000000302ULL,
    0x0000000000000705ULL,
    0x0000000000000e0aULL,
    0x0000000000001c14ULL,
    0x0000000000003828ULL,
    0x0000000000007050ULL,
    0x000000000000e0a0ULL,
    0x000000000000c040ULL,
    0x0000000000030203ULL,
    0x0000000000070507ULL,
    0x00000000000e0a0eULL,
    0x00000000001c141cULL,
    0x0000000000382838ULL,
    0x0000000000705070ULL,
    0x0000000000e0a0e0ULL,
    0x0000000000c040c0ULL,
    0x0000000003020300ULL,
    0x0000000007050700ULL,
    0x000000000e0a0e00ULL,
    0x000000001c141c00ULL,
    0x0000000038283800ULL,
    0x0000000070507000ULL,
    0x00000000e0a0e000ULL,
    0x00000000c040c000ULL,
    0x0000000302030000ULL,
    0x0000000705070000ULL,
    0x0000000e0a0e0000ULL,
    0x0000001c141c0000ULL,
    0x0000003828380000ULL,
    0x0000007050700000ULL,
    0x000000e0a0e00000ULL,
    0x000000c040c00000ULL,
    0x0000030203000000ULL,
    0x0000070507000000ULL,
    0x00000e0a0e000000ULL,
    0x00001c141c000000ULL,
    0x0000382838000000ULL,
    0x0000705070000000ULL,
    0x0000e0a0e0000000ULL,
    0x0000c040c0000000ULL,
    0x0003020300000000ULL,
    0x0007050700000000ULL,
    0x000e0a0e00000000ULL,
    0x001c141c00000000ULL,
    0x0038283800000000ULL,
    0x0070507000000000ULL,
    0x00e0a0e000000000ULL,
    0x00c040c000000000ULL,
    0x0302030000000000ULL,
    0x0705070000000000ULL,
    0x0e0a0e0000000000ULL,
    0x1c141c0000000000ULL,
    0x3828380000000000ULL,
    0x7050700000000000ULL,
    0xe0a0e00000000000ULL,
    0xc040c00000000000ULL,
    0x0203000000000000ULL,
    0x0507000000000000ULL,
    0x0a0e000000000000ULL,
    0x141c000000000000ULL,
    0x2838000000000000ULL,
    0x5070000000000000ULL,
    0xa0e0000000000000ULL,
    0x40c0000000000000ULL,
};

Bitboard getKingAttacks(int square)
{
    return king_attacks[square];
}