#include "sliding_attacks.h"

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