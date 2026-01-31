#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sliding_attacks.h"
#include "bitboard.h"

// Generate a random 64-bit number with few bits set (sparse)
static Bitboard randomBitboard()
{
    Bitboard u1, u2, u3, u4;
    u1 = (Bitboard)(rand()) & 0xFFFF;
    u2 = (Bitboard)(rand()) & 0xFFFF;
    u3 = (Bitboard)(rand()) & 0xFFFF;
    u4 = (Bitboard)(rand()) & 0xFFFF;
    return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

static Bitboard randomBitboardFewBits()
{
    return randomBitboard() & randomBitboard() & randomBitboard();
}

// Transform function for magic bitboards
static inline int transform(Bitboard occupancy, Bitboard magic, int bits)
{
    return (int)((occupancy * magic) >> (64 - bits));
}

// Try to find a magic number for a given square
static Bitboard findMagic(int square, int bits, int isBishop)
{
    Bitboard mask = isBishop ? bishop_occupancy_maps[square] : rook_occupancy_maps[square];
    int n = popCount(mask);
    int permutations = 1 << n;

    Bitboard occupancies[4096];
    Bitboard attacks[4096];
    Bitboard used[4096];

    // Generate all occupancy variations
    for (int i = 0; i < permutations; i++)
    {
        Bitboard occupancy = 0ULL;
        Bitboard temp_mask = mask;
        int count = 0;

        while (temp_mask)
        {
            int bit_pos = __builtin_ctzll(temp_mask);
            if (i & (1 << count))
            {
                occupancy |= (1ULL << bit_pos);
            }
            temp_mask &= temp_mask - 1;
            count++;
        }

        occupancies[i] = occupancy;
        attacks[i] = isBishop ? generateBishopAttacks(square, occupancy) : generateRookAttacks(square, occupancy);
    }

    // Try random magics until we find one that works
    for (int k = 0; k < 100000000; k++)
    {
        Bitboard magic = randomBitboardFewBits();

        // Skip unsuitable magics early
        if (popCount((mask * magic) & 0xFF00000000000000ULL) < 6)
            continue;

        // Clear used array
        for (int i = 0; i < (1 << bits); i++)
        {
            used[i] = 0ULL;
        }

        // Test if this magic works for all occupancies
        int fail = 0;
        for (int i = 0; i < permutations && !fail; i++)
        {
            int index = transform(occupancies[i], magic, bits);

            if (used[index] == 0ULL)
            {
                used[index] = attacks[i];
            }
            else if (used[index] != attacks[i])
            {
                fail = 1;
            }
        }

        if (!fail)
        {
            return magic;
        }

        if (k % 1000000 == 0 && k > 0)
        {
            printf(".");
            fflush(stdout);
        }
    }

    printf("\nFailed to find magic for square %d\n", square);
    return 0ULL;
}

void generateAllMagics()
{
    srand(time(NULL));

    printf("Generating Bishop Magics...\n");
    printf("const Bitboard BMagic[64] = {\n");
    for (int square = 0; square < 64; square++)
    {
        int bits = BBits[square];
        Bitboard magic = findMagic(square, bits, 1);
        printf("    0x%llxULL,", magic);
        if (square % 4 == 3)
            printf("\n");
        else
            printf(" ");
        fflush(stdout);
    }
    printf("};\n\n");

    printf("Generating Rook Magics...\n");
    printf("const Bitboard RMagic[64] = {\n");
    for (int square = 0; square < 64; square++)
    {
        int bits = RBits[square];
        Bitboard magic = findMagic(square, bits, 0);
        printf("    0x%llxULL,", magic);
        if (square % 4 == 3)
            printf("\n");
        else
            printf(" ");
        fflush(stdout);
    }
    printf("};\n");
}

int main()
{
    generateAllMagics();
    return 0;
}
