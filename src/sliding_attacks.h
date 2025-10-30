#include "bitboard.h"

static const Bitboard rook_occupancy_maps[] = {
    0x000101010101017eULL,
    0x000202020202027cULL,
    0x000404040404047aULL,
    0x0008080808080876ULL,
    0x001010101010106eULL,
    0x002020202020205eULL,
    0x004040404040403eULL,
    0x008080808080807eULL,
    0x0001010101017e00ULL,
    0x0002020202027c00ULL,
    0x0004040404047a00ULL,
    0x0008080808087600ULL,
    0x0010101010106e00ULL,
    0x0020202020205e00ULL,
    0x0040404040403e00ULL,
    0x0080808080807e00ULL,
    0x00010101017e0100ULL,
    0x00020202027c0200ULL,
    0x00040404047a0400ULL,
    0x0008080808760800ULL,
    0x00101010106e1000ULL,
    0x00202020205e2000ULL,
    0x00404040403e4000ULL,
    0x00808080807e8000ULL,
    0x000101017e010100ULL,
    0x000202027c020200ULL,
    0x000404047a040400ULL,
    0x0008080876080800ULL,
    0x001010106e101000ULL,
    0x002020205e202000ULL,
    0x004040403e404000ULL,
    0x008080807e808000ULL,
    0x0001017e01010100ULL,
    0x0002027c02020200ULL,
    0x0004047a04040400ULL,
    0x0008087608080800ULL,
    0x0010106e10101000ULL,
    0x0020205e20202000ULL,
    0x0040403e40404000ULL,
    0x0080807e80808000ULL,
    0x00017e0101010100ULL,
    0x00027c0202020200ULL,
    0x00047a0404040400ULL,
    0x0008760808080800ULL,
    0x00106e1010101000ULL,
    0x00205e2020202000ULL,
    0x00403e4040404000ULL,
    0x00807e8080808000ULL,
    0x007e010101010100ULL,
    0x007c020202020200ULL,
    0x007a040404040400ULL,
    0x0076080808080800ULL,
    0x006e101010101000ULL,
    0x005e202020202000ULL,
    0x003e404040404000ULL,
    0x007e808080808000ULL,
    0x7e01010101010100ULL,
    0x7c02020202020200ULL,
    0x7a04040404040400ULL,
    0x7608080808080800ULL,
    0x6e10101010101000ULL,
    0x5e20202020202000ULL,
    0x3e40404040404000ULL,
    0x7e80808080808000ULL,
};

static const Bitboard bishop_occupancy_maps[] = {
    0x0040201008040200ULL,
    0x0000402010080400ULL,
    0x0000004020100a00ULL,
    0x0000000040221400ULL,
    0x0000000002442800ULL,
    0x0000000204085000ULL,
    0x0000020408102000ULL,
    0x0002040810204000ULL,
    0x0020100804020000ULL,
    0x0040201008040000ULL,
    0x00004020100a0000ULL,
    0x0000004022140000ULL,
    0x0000000244280000ULL,
    0x0000020408500000ULL,
    0x0002040810200000ULL,
    0x0004081020400000ULL,
    0x0010080402000200ULL,
    0x0020100804000400ULL,
    0x004020100a000a00ULL,
    0x0000402214001400ULL,
    0x0000024428002800ULL,
    0x0002040850005000ULL,
    0x0004081020002000ULL,
    0x0008102040004000ULL,
    0x0008040200020400ULL,
    0x0010080400040800ULL,
    0x0020100a000a1000ULL,
    0x0040221400142200ULL,
    0x0002442800284400ULL,
    0x0004085000500800ULL,
    0x0008102000201000ULL,
    0x0010204000402000ULL,
    0x0004020002040800ULL,
    0x0008040004081000ULL,
    0x00100a000a102000ULL,
    0x0022140014224000ULL,
    0x0044280028440200ULL,
    0x0008500050080400ULL,
    0x0010200020100800ULL,
    0x0020400040201000ULL,
    0x0002000204081000ULL,
    0x0004000408102000ULL,
    0x000a000a10204000ULL,
    0x0014001422400000ULL,
    0x0028002844020000ULL,
    0x0050005008040200ULL,
    0x0020002010080400ULL,
    0x0040004020100800ULL,
    0x0000020408102000ULL,
    0x0000040810204000ULL,
    0x00000a1020400000ULL,
    0x0000142240000000ULL,
    0x0000284402000000ULL,
    0x0000500804020000ULL,
    0x0000201008040200ULL,
    0x0000402010080400ULL,
    0x0002040810204000ULL,
    0x0004081020400000ULL,
    0x000a102040000000ULL,
    0x0014224000000000ULL,
    0x0028440200000000ULL,
    0x0050080402000000ULL,
    0x0020100804020000ULL,
    0x0040201008040200ULL,
};
// the following functions were used to generate the occupancy maps
// void computeRookOccupancyMaps()
// {
//     for (int square = 0; square < 64; square++)
//     {
//         int rank = square / 8;
//         int file = square % 8;

//         /* squares on same rank, excluding edge files A and H */
//         Bitboard rankMask = RANKS[rank] & ~(FILE_A | FILE_H);

//         /* squares on same file, excluding edge ranks 1 and 8 */
//         Bitboard fileMask = FILES[file] & ~(RANK_1 | RANK_8);

//         /* combine and remove the current square */
//         Bitboard mask = (rankMask | fileMask) & ~square_mask(square);

//         printf("0x%016llxULL,\n", mask);
//     }
// }
// void computeBishopOccupancyMaps()
// {
//     printf("static const Bitboard bishop_occupancy_maps[] = {\n");
//     for (int sq = 0; sq < 64; sq++)
//     {
//         int rank = sq / 8;
//         int file = sq % 8;
//         Bitboard mask = 0ULL;

//         // NE
//         for (int r = rank + 1, f = file + 1; r <= 6 && f <= 6; r++, f++)
//             mask |= 1ULL << (r * 8 + f);
//         // NW
//         for (int r = rank + 1, f = file - 1; r <= 6 && f >= 1; r++, f--)
//             mask |= 1ULL << (r * 8 + f);
//         // SE
//         for (int r = rank - 1, f = file + 1; r >= 1 && f <= 6; r--, f++)
//             mask |= 1ULL << (r * 8 + f);
//         // SW
//         for (int r = rank - 1, f = file - 1; r >= 1 && f >= 1; r--, f--)
//             mask |= 1ULL << (r * 8 + f);
//         printf("    0x%016llxULL,%s", mask, (sq + 1) % 8 == 0 ? "\n" : " ");
//     }
//     printf("};\n");
// }
const Bitboard RMagic[64] = {
    0x2080020500400f0ULL,
    0x28444000400010ULL,
    0x20000a1004100014ULL,
    0x20010c090202006ULL,
    0x8408008200810004ULL,
    0x1746000808002ULL,
    0x2200098000808201ULL,
    0x12c0002080200041ULL,
    0x104000208e480804ULL,
    0x8084014008281008ULL,
    0x4200810910500410ULL,
    0x100014481c20400cULL,
    0x4014a4040020808ULL,
    0x401002001010a4ULL,
    0x202000500010001ULL,
    0x8112808005810081ULL,
    0x40902108802020ULL,
    0x42002101008101ULL,
    0x459442200810c202ULL,
    0x81001103309808ULL,
    0x8110000080102ULL,
    0x8812806008080404ULL,
    0x104020000800101ULL,
    0x40a1048000028201ULL,
    0x4100ba0000004081ULL,
    0x44803a4003400109ULL,
    0xa010a00000030443ULL,
    0x91021a000100409ULL,
    0x4201e8040880a012ULL,
    0x22a000440201802ULL,
    0x30890a72000204ULL,
    0x10411402a0c482ULL,
    0x40004841102088ULL,
    0x40230000100040ULL,
    0x40100010000a0488ULL,
    0x1410100200050844ULL,
    0x100090808508411ULL,
    0x1410040024001142ULL,
    0x8840018001214002ULL,
    0x410201000098001ULL,
    0x8400802120088848ULL,
    0x2060080000021004ULL,
    0x82101002000d0022ULL,
    0x1001101001008241ULL,
    0x9040411808040102ULL,
    0x600800480009042ULL,
    0x1a020000040205ULL,
    0x4200404040505199ULL,
    0x2020081040080080ULL,
    0x40a3002000544108ULL,
    0x4501100800148402ULL,
    0x81440280100224ULL,
    0x88008000000804ULL,
    0x8084060000002812ULL,
    0x1840201000108312ULL,
    0x5080202000000141ULL,
    0x1042a180880281ULL,
    0x900802900c01040ULL,
    0x8205104104120ULL,
    0x9004220000440aULL,
    0x8029510200708ULL,
    0x8008440100404241ULL,
    0x2420001111000bdULL,
    0x4000882304000041ULL,
};

const Bitboard BMagic[64] = {
    0x100420000431024ULL,
    0x280800101073404ULL,
    0x42000a00840802ULL,
    0xca800c0410c2ULL,
    0x81004290941c20ULL,
    0x400200450020250ULL,
    0x444a019204022084ULL,
    0x88610802202109aULL,
    0x11210a0800086008ULL,
    0x400a08c08802801ULL,
    0x1301a0500111c808ULL,
    0x1280100480180404ULL,
    0x720009020028445ULL,
    0x91880a9000010a01ULL,
    0x31200940150802b2ULL,
    0x5119080c20000602ULL,
    0x242400a002448023ULL,
    0x4819006001200008ULL,
    0x222c10400020090ULL,
    0x302008420409004ULL,
    0x504200070009045ULL,
    0x210071240c02046ULL,
    0x1182219000022611ULL,
    0x400c50000005801ULL,
    0x4004010000113100ULL,
    0x2008121604819400ULL,
    0xc4a4010000290101ULL,
    0x404a000888004802ULL,
    0x8820c004105010ULL,
    0x28280100908300ULL,
    0x4c013189c0320a80ULL,
    0x42008080042080ULL,
    0x90803000c080840ULL,
    0x2180001028220ULL,
    0x1084002a040036ULL,
    0x212009200401ULL,
    0x128110040c84a84ULL,
    0x81488020022802ULL,
    0x8c0014100181ULL,
    0x2222013020082ULL,
    0xa00100002382c03ULL,
    0x1000280001005c02ULL,
    0x84801010000114cULL,
    0x480410048000084ULL,
    0x21204420080020aULL,
    0x2020010000424a10ULL,
    0x240041021d500141ULL,
    0x420844000280214ULL,
    0x29084a280042108ULL,
    0x84102a8080a20a49ULL,
    0x104204908010212ULL,
    0x40a20280081860c1ULL,
    0x3044000200121004ULL,
    0x1001008807081122ULL,
    0x50066c000210811ULL,
    0xe3001240f8a106ULL,
    0x940c0204030020d4ULL,
    0x619204000210826aULL,
    0x2010438002b00a2ULL,
    0x884042004005802ULL,
    0xa90240000006404ULL,
    0x500d082244010008ULL,
    0x28190d00040014e0ULL,
    0x825201600c082444ULL,
};

// the above magic numbers were computed with the following code, courtesy of
// https://www.chessprogramming.org/Looking_for_Magics
// Bitboard random_Bitboard()
// {
//     Bitboard u1, u2, u3, u4;
//     u1 = (Bitboard)(random()) & 0xFFFF;
//     u2 = (Bitboard)(random()) & 0xFFFF;
//     u3 = (Bitboard)(random()) & 0xFFFF;
//     u4 = (Bitboard)(random()) & 0xFFFF;
//     return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
// }

// Bitboard random_Bitboard_fewbits()
// {
//     return random_Bitboard() & random_Bitboard() & random_Bitboard();
// }

// int count_1s(Bitboard b)
// {
//     int r;
//     for (r = 0; b; r++, b &= b - 1)
//         ;
//     return r;
// }

// const int BitTable[64] = {
//     63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
//     51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
//     26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
//     58, 20, 37, 17, 36, 8};

// int pop_1st_bit(Bitboard *bb)
// {
//     Bitboard b = *bb ^ (*bb - 1);
//     unsigned int fold = (unsigned)((b & 0xffffffff) ^ (b >> 32));
//     *bb &= (*bb - 1);
//     return BitTable[(fold * 0x783a9b23) >> 26];
// }

// Bitboard index_to_Bitboard(int index, int bits, Bitboard m)
// {
//     int i, j;
//     Bitboard result = 0ULL;
//     for (i = 0; i < bits; i++)
//     {
//         j = pop_1st_bit(&m);
//         if (index & (1 << i))
//             result |= (1ULL << j);
//     }
//     return result;
// }

// Bitboard rmask(int sq)
// {
//     Bitboard result = 0ULL;
//     int rk = sq / 8, fl = sq % 8, r, f;
//     for (r = rk + 1; r <= 6; r++)
//         result |= (1ULL << (fl + r * 8));
//     for (r = rk - 1; r >= 1; r--)
//         result |= (1ULL << (fl + r * 8));
//     for (f = fl + 1; f <= 6; f++)
//         result |= (1ULL << (f + rk * 8));
//     for (f = fl - 1; f >= 1; f--)
//         result |= (1ULL << (f + rk * 8));
//     return result;
// }

// Bitboard bmask(int sq)
// {
//     Bitboard result = 0ULL;
//     int rk = sq / 8, fl = sq % 8, r, f;
//     for (r = rk + 1, f = fl + 1; r <= 6 && f <= 6; r++, f++)
//         result |= (1ULL << (f + r * 8));
//     for (r = rk + 1, f = fl - 1; r <= 6 && f >= 1; r++, f--)
//         result |= (1ULL << (f + r * 8));
//     for (r = rk - 1, f = fl + 1; r >= 1 && f <= 6; r--, f++)
//         result |= (1ULL << (f + r * 8));
//     for (r = rk - 1, f = fl - 1; r >= 1 && f >= 1; r--, f--)
//         result |= (1ULL << (f + r * 8));
//     return result;
// }

// Bitboard ratt(int sq, Bitboard block)
// {
//     Bitboard result = 0ULL;
//     int rk = sq / 8, fl = sq % 8, r, f;
//     for (r = rk + 1; r <= 7; r++)
//     {
//         result |= (1ULL << (fl + r * 8));
//         if (block & (1ULL << (fl + r * 8)))
//             break;
//     }
//     for (r = rk - 1; r >= 0; r--)
//     {
//         result |= (1ULL << (fl + r * 8));
//         if (block & (1ULL << (fl + r * 8)))
//             break;
//     }
//     for (f = fl + 1; f <= 7; f++)
//     {
//         result |= (1ULL << (f + rk * 8));
//         if (block & (1ULL << (f + rk * 8)))
//             break;
//     }
//     for (f = fl - 1; f >= 0; f--)
//     {
//         result |= (1ULL << (f + rk * 8));
//         if (block & (1ULL << (f + rk * 8)))
//             break;
//     }
//     return result;
// }

// Bitboard batt(int sq, Bitboard block)
// {
//     Bitboard result = 0ULL;
//     int rk = sq / 8, fl = sq % 8, r, f;
//     for (r = rk + 1, f = fl + 1; r <= 7 && f <= 7; r++, f++)
//     {
//         result |= (1ULL << (f + r * 8));
//         if (block & (1ULL << (f + r * 8)))
//             break;
//     }
//     for (r = rk + 1, f = fl - 1; r <= 7 && f >= 0; r++, f--)
//     {
//         result |= (1ULL << (f + r * 8));
//         if (block & (1ULL << (f + r * 8)))
//             break;
//     }
//     for (r = rk - 1, f = fl + 1; r >= 0 && f <= 7; r--, f++)
//     {
//         result |= (1ULL << (f + r * 8));
//         if (block & (1ULL << (f + r * 8)))
//             break;
//     }
//     for (r = rk - 1, f = fl - 1; r >= 0 && f >= 0; r--, f--)
//     {
//         result |= (1ULL << (f + r * 8));
//         if (block & (1ULL << (f + r * 8)))
//             break;
//     }
//     return result;
// }

// int transform(Bitboard b, Bitboard magic, int bits)
// {
// #if defined(USE_32_BIT_MULTIPLICATIONS)
//     return (unsigned)((int)b * (int)magic ^ (int)(b >> 32) * (int)(magic >> 32)) >> (32 - bits);
// #else
//     return (int)((b * magic) >> (64 - bits));
// #endif
// }

// Bitboard find_magic(int sq, int m, int bishop)
// {
//     Bitboard mask, b[4096], a[4096], used[4096], magic;
//     int i, j, k, n, fail;

//     mask = bishop ? bmask(sq) : rmask(sq);
//     n = count_1s(mask);

//     for (i = 0; i < (1 << n); i++)
//     {
//         b[i] = index_to_Bitboard(i, n, mask);
//         a[i] = bishop ? batt(sq, b[i]) : ratt(sq, b[i]);
//     }
//     for (k = 0; k < 100000000; k++)
//     {
//         magic = random_Bitboard_fewbits();
//         if (count_1s((mask * magic) & 0xFF00000000000000ULL) < 6)
//             continue;
//         for (i = 0; i < 4096; i++)
//             used[i] = 0ULL;
//         for (i = 0, fail = 0; !fail && i < (1 << n); i++)
//         {
//             j = transform(b[i], magic, m);
//             if (used[j] == 0ULL)
//                 used[j] = a[i];
//             else if (used[j] != a[i])
//                 fail = 1;
//         }
//         if (!fail)
//             return magic;
//     }
//     printf("***Failed***\n");
//     return 0ULL;
// }

// int RBits[64] = {
//     12, 11, 11, 11, 11, 11, 11, 12,
//     11, 10, 10, 10, 10, 10, 10, 11,
//     11, 10, 10, 10, 10, 10, 10, 11,
//     11, 10, 10, 10, 10, 10, 10, 11,
//     11, 10, 10, 10, 10, 10, 10, 11,
//     11, 10, 10, 10, 10, 10, 10, 11,
//     11, 10, 10, 10, 10, 10, 10, 11,
//     12, 11, 11, 11, 11, 11, 11, 12};

// int BBits[64] = {
//     6, 5, 5, 5, 5, 5, 5, 6,
//     5, 5, 5, 5, 5, 5, 5, 5,
//     5, 5, 7, 7, 7, 7, 5, 5,
//     5, 5, 7, 9, 9, 7, 5, 5,
//     5, 5, 7, 9, 9, 7, 5, 5,
//     5, 5, 7, 7, 7, 7, 5, 5,
//     5, 5, 5, 5, 5, 5, 5, 5,
//     6, 5, 5, 5, 5, 5, 5, 6};

// int main()
// {
//     int square;

//     printf("const Bitboard RMagic[64] = {\n");
//     for (square = 0; square < 64; square++)
//         printf("  0x%llxULL,\n", find_magic(square, RBits[square], 0));
//     printf("};\n\n");

//     printf("const Bitboard BMagic[64] = {\n");
//     for (square = 0; square < 64; square++)
//         printf("  0x%llxULL,\n", find_magic(square, BBits[square], 1));
//     printf("};\n\n");

//     return 0;
// }