#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#ifndef BITBOARD_H
#define BITBOARD_H
// A1 = LSB, H8 = MSB, little-endian rank-file mapping
typedef uint64_t Bitboard;

// much of the code below comes from the chessprograming wiki
// https://www.chessprogramming.org/Population_Count

// is bitboard empty function
static inline bool isBitboardEmpty(Bitboard bb)
{
    return bb == 0;
}

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
    return popCount((bb & -bb) - 1);
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
    int idx = __builtin_ctzll(*b);
    *b &= *b - 1;
    return idx;
}

// squares enum
typedef enum
{
    A1,
    B1,
    C1,
    D1,
    E1,
    F1,
    G1,
    H1,
    A2,
    B2,
    C2,
    D2,
    E2,
    F2,
    G2,
    H2,
    A3,
    B3,
    C3,
    D3,
    E3,
    F3,
    G3,
    H3,
    A4,
    B4,
    C4,
    D4,
    E4,
    F4,
    G4,
    H4,
    A5,
    B5,
    C5,
    D5,
    E5,
    F5,
    G5,
    H5,
    A6,
    B6,
    C6,
    D6,
    E6,
    F6,
    G6,
    H6,
    A7,
    B7,
    C7,
    D7,
    E7,
    F7,
    G7,
    H7,
    A8,
    B8,
    C8,
    D8,
    E8,
    F8,
    G8,
    H8
} Square;

typedef enum
{
    white,
    black
} Color;

// map (rank 0-7, file 0-7) → bit index 0-63
static inline int square_index(int rank, int file)
{
    return rank * 8 + file;
}

// build a bitboard with exactly that square set
static inline Bitboard BB(int rank, int file)
{
    return (Bitboard)1 << square_index(rank, file);
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

#endif // BITBOARD_H