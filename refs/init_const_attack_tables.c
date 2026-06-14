// void initKnightAttacks()
// {
//     for (int sq = 0; sq < 64; sq++) {
//         Bitboard attacks = 0;
//         int rank = sq / 8;
//         int file = sq % 8;

//         int knight_moves[8][2] = {
//             { 2, 1 }, { 1, 2 }, { -1, 2 }, { -2, 1 }, { -2, -1 }, { -1, -2 }, { 1, -2 }, { 2, -1 }
//         };

//         for (int i = 0; i < 8; i++) {
//             int new_rank = rank + knight_moves[i][0];
//             int new_file = file + knight_moves[i][1];

//             if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
//                 attacks |= ((Bitboard)1 << (new_rank * 8 + new_file));
//             }
//         }
//         knight_attacks[sq] = attacks;
//     }
//     // print knight attacks for storage as constant array
//     printf("const Bitboard knight_attacks[64] = {\n");
//     for (int i = 0; i < 64; i++) {
//         printf("    0x%016llxULL,%s\n", knight_attacks[i], (i % 8 == 7) ? "" : " ");
//     }
//     printf("};\n");
// }

// void initWhitePawnAttacks()
// {
//     for (int sq = 0; sq < 64; sq++) {
//         Bitboard attacks = 0;
//         int rank = sq / 8;
//         int file = sq % 8;

//         // white pawn attacks diagonally forward left and right
//         int attack_moves[2][2] = {
//             { 1, -1 }, { 1, 1 }
//         };

//         for (int i = 0; i < 2; i++) {
//             int new_rank = rank + attack_moves[i][0];
//             int new_file = file + attack_moves[i][1];

//             if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
//                 attacks |= ((Bitboard)1 << (new_rank * 8 + new_file));
//             }
//         }
//         pawn_attacks_white[sq] = attacks;
//     }
//     printf("const Bitboard white_pawn_attacks[64] = {\n");
//     for (int i = 0; i < 64; i++) {
//         printf("    0x%016llxULL,%s\n", pawn_attacks_white[i], (i % 8 == 7) ? "" : " ");
//     }
//     printf("};\n");
// }

// void initBlackPawnAttacks()
// {
//     for (int sq = 64; sq < 64; sq++) {
//         Bitboard attacks = 0;
//         int rank = sq / 8;
//         int file = sq % 8;

//         // black pawn attacks diagonally forward left and right
//         int attack_moves[2][2] = {
//             { -1, -1 }, { -1, 1 }
//         };

//         for (int i = 0; i < 2; i++) {
//             int new_rank = rank + attack_moves[i][0];
//             int new_file = file + attack_moves[i][1];

//             if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
//                 attacks |= ((Bitboard)1 << (new_rank * 8 + new_file));
//             }
//         }
//         pawn_attacks_black[sq] = attacks;
//     }
//     printf("const Bitboard black_pawn_attacks[64] = {\n");
//     for (int i = 0; i < 64; i++) {
//         printf("    0x%016llxULL,%s\n", pawn_attacks_black[i], (i % 8 == 7) ? "" : " ");
//     }
//     printf("};\n");
// }

// void initKingAttacks()
// {
//     for (int sq = 0; sq < 64; sq++) {
//         Bitboard attacks = 0;
//         int rank = sq / 8;
//         int file = sq % 8;

//         int king_moves[8][2] = {
//             { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 }
//         };

//         for (int i = 0; i < 8; i++) {
//             int new_rank = rank + king_moves[i][0];
//             int new_file = file + king_moves[i][1];

//             if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
//                 attacks |= ((Bitboard)1 << (new_rank * 8 + new_file));
//             }
//         }
//         king_attacks[sq] = attacks;
//     }
//     printf("const Bitboard king_attacks[64] = {\n");
//     for (int i = 0; i < 64; i++) {
//         printf("    0x%016llxULL,%s\n", king_attacks[i], (i % 8 == 7) ? "" : " ");
//     }
//     printf("};\n");
// }