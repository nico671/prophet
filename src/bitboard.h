#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "chess_types.h"
#ifndef BITBOARD_H
#define BITBOARD_H
// A1 = LSB, H8 = MSB, little-endian rank-file mapping
typedef unsigned long long Bitboard;

// much of the code below comes from the chessprograming wiki
// https://www.chessprogramming.org/Population_Count

// is bitboard empty function
static inline bool isBitboardEmpty(Bitboard bb)
{
    return bb == 0;
}
// shoutout chessprogramming wiki
static const uint64_t K1 = 0x5555555555555555ULL; /*  -1/3   */
static const uint64_t K2 = 0x3333333333333333ULL; /*  -1/5   */
static const uint64_t K4 = 0x0f0f0f0f0f0f0f0fULL; /*  -1/17  */
static const uint64_t KF = 0x0101010101010101ULL; /*  -1/255 */

static inline int popCount(Bitboard bb)
{
    bb = bb - ((bb >> 1) & K1);        // put count of each 2 bits into those 2 bits
    bb = (bb & K2) + ((bb >> 2) & K2); // put count of each 4 bits into those 4 bits
    bb = (bb + (bb >> 4)) & K4;        // put count of each 8 bits into those 8 bits
    return (int)((bb * KF) >> 56);     // returns left 8 bits of bb + (bb<<8) + (bb<<16) + ... + (bb<<56)
}

// returns index (0-63) of least significant 1 bit
// precondition: bb != 0
static inline int bitboardLSBIndex(Bitboard bb)
{
    return __builtin_ctzll(bb);
}

// returns index (0-63) of most significant 1 bit
// precondition: bb != 0
static inline int bitboardMSBIndex(Bitboard bb)
{
    bb |= (bb >> 1);
    bb |= (bb >> 2);
    bb |= (bb >> 4);
    bb |= (bb >> 8);
    bb |= (bb >> 16);
    bb |= (bb >> 32);
    return popCount(bb >> 1);
}

// Pop & return index of the least‐significant 1-bit
static inline int bb_pop_lsb(Bitboard *b)
{
    if (isBitboardEmpty(*b))
        return NO_SQUARE;
    int idx = __builtin_ctzll(*b); // count trailing zeros, built-in function
    *b &= *b - 1;
    return idx;
}

// build a bitboard with exactly that square set
static inline Bitboard BB(int rank, int file)
{
    return (Bitboard)1 << (rank * 8 + file);
}

// Test whether bitboard b has bit sq set (0 or 1)
static inline int bb_test(Bitboard b, int sq)
{
    return (b >> sq) & 1;
}

// Set bit sq in *b
static inline void bb_set(Bitboard *b, int sq)
{
    *b |= (Bitboard)1 << sq;
}

// Clear bit sq in *b
static inline void bb_clear(Bitboard *b, int sq)
{
    *b &= ~((Bitboard)1 << sq);
}

static inline void printBBoard(Bitboard b)
{
    for (int rank = 7; rank >= 0; rank--)
    {
        for (int file = 0; file < 8; file++)
        {
            int squareIndex = rank * 8 + file;
            char pieceChar = bb_test(b, squareIndex) ? '1' : '.';
            printf("%c ", pieceChar);
        }
        printf("\n");
    }
    printf("\n");
}

// get square index of ith bit in bitboard b
// precondition: i < popCount(b)
static inline int bb_get_ith_square(Bitboard b, int i)
{
    Bitboard temp = b;
    for (int j = 0; j < i; j++)
    {
        temp &= temp - 1; // clear least significant bit
    }
    return bitboardLSBIndex(temp);
}

/* file masks (A..H) and complements */
static const Bitboard FILE_A = 0x0101010101010101ULL;
static const Bitboard FILE_B = FILE_A << 1;
static const Bitboard FILE_C = FILE_A << 2;
static const Bitboard FILE_D = FILE_A << 3;
static const Bitboard FILE_E = FILE_A << 4;
static const Bitboard FILE_F = FILE_A << 5;
static const Bitboard FILE_G = FILE_A << 6;
static const Bitboard FILE_H = FILE_A << 7;

static const Bitboard FILES[8] = {
    FILE_A, FILE_B, FILE_C, FILE_D,
    FILE_E, FILE_F, FILE_G, FILE_H};

static const Bitboard NOT_FILE_A = ~FILE_A;
static const Bitboard NOT_FILE_H = ~FILE_H;
static const Bitboard NOT_FILE_AB = ~(FILE_A | FILE_B);
static const Bitboard NOT_FILE_GH = ~(FILE_G | FILE_H);

/* rank masks (1..8) */
static const Bitboard RANK_1 = 0x00000000000000FFULL;
static const Bitboard RANK_2 = RANK_1 << 8;
static const Bitboard RANK_3 = RANK_1 << 16;
static const Bitboard RANK_4 = RANK_1 << 24;
static const Bitboard RANK_5 = RANK_1 << 32;
static const Bitboard RANK_6 = RANK_1 << 40;
static const Bitboard RANK_7 = RANK_1 << 48;
static const Bitboard RANK_8 = RANK_1 << 56;
static const Bitboard RANKS[8] = {
    RANK_1, RANK_2, RANK_3, RANK_4,
    RANK_5, RANK_6, RANK_7, RANK_8};
/* build a mask for a single square by index */
static inline Bitboard square_mask(int sq)
{
    return (Bitboard)1 << sq;
}

/* Safe directional shifts (no wraps) */
static inline Bitboard north(Bitboard b) { return b << 8; }
static inline Bitboard south(Bitboard b) { return b >> 8; }
static inline Bitboard east(Bitboard b) { return (b << 1) & NOT_FILE_A; } // file++ (mask out wraps into file a)
static inline Bitboard west(Bitboard b) { return (b >> 1) & NOT_FILE_H; } // file-- (mask out wraps into file h)
static inline Bitboard north_east(Bitboard b) { return (b << 9) & NOT_FILE_A; }
static inline Bitboard north_west(Bitboard b) { return (b << 7) & NOT_FILE_H; }
static inline Bitboard south_east(Bitboard b) { return (b >> 7) & NOT_FILE_A; }
static inline Bitboard south_west(Bitboard b) { return (b >> 9) & NOT_FILE_H; }

#endif // BITBOARD_H