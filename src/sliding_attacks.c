#include "sliding_attacks.h"

const Bitboard RMagic[64] = {
    0x800040008a6111ULL,
    0x240004010002005ULL,
    0x1080100084082000ULL,
    0x2080080010000680ULL,
    0x2a00020060042890ULL,
    0x100040002080100ULL,
    0x4200050850860004ULL,
    0x100004080220100ULL,
    0x8200800040008020ULL,
    0x9808802004824000ULL,
    0x400808010002000ULL,
    0x1021001000210008ULL,
    0x3201800801240080ULL,
    0x602000810040200ULL,
    0x2000881020004ULL,
    0x1000041003082ULL,
    0x828000400120ULL,
    0x10004010402004ULL,
    0x8200410013022002ULL,
    0xc008808010000802ULL,
    0x24c0808004000800ULL,
    0x4c808004010200ULL,
    0x88808001000200ULL,
    0x214a0004144081ULL,
    0x24000a180008040ULL,
    0x4020002440005000ULL,
    0x21e008200401020ULL,
    0x10004040080400ULL,
    0x9260050100080011ULL,
    0xc806000901000400ULL,
    0x8020400411008ULL,
    0x32002242000c0291ULL,
    0x480004000402004ULL,
    0x201000404000ULL,
    0x802000801000ULL,
    0x980801000800804ULL,
    0x8040080800800ULL,
    0x15800200800400ULL,
    0x8100021004000801ULL,
    0x440208402000041ULL,
    0x884008288000ULL,
    0x20008040010100ULL,
    0xa0002010008080ULL,
    0x110102200420008ULL,
    0x2000040801010010ULL,
    0x20004008080ULL,
    0x2000901231040038ULL,
    0x20400400419a0005ULL,
    0x2c80002000400040ULL,
    0x80248040011300ULL,
    0x400100480200880ULL,
    0x601001000082100ULL,
    0x2000420081200ULL,
    0x40080020080ULL,
    0x4080102100400ULL,
    0x404040041008200ULL,
    0xa1004b108001ULL,
    0x120020410082ULL,
    0x210200a024082ULL,
    0x50214c810002101ULL,
    0x690200041020182aULL,
    0x402001008810402ULL,
    0x400010210080084ULL,
    0x1000002410410082ULL,
};

const Bitboard BMagic[64] = {
    0x22081060c8c0c80ULL,
    0x2103400888002ULL,
    0x2018860402a22380ULL,
    0x4040084098810ULL,
    0x2021018000308ULL,
    0x2001101952010401ULL,
    0x100c040202122000ULL,
    0x8000104202104001ULL,
    0x2100100410409201ULL,
    0x40020202040900ULL,
    0x30500420405000ULL,
    0x9500145400820004ULL,
    0x8021840420080440ULL,
    0x4211020882280008ULL,
    0x28100a0084606848ULL,
    0xc004042101108ULL,
    0x6412a060e0020080ULL,
    0x242601002281901ULL,
    0x10188800821010ULL,
    0x880a0082004404ULL,
    0x88880400a00010ULL,
    0x2010100820102ULL,
    0x2004000094010800ULL,
    0x812003100a20119ULL,
    0x20208864090640ULL,
    0x1040509080808ULL,
    0xa24020811c400ULL,
    0x1214080005005100ULL,
    0x421010041444000ULL,
    0x40d0008083004562ULL,
    0x428801000084410bULL,
    0x8444082240410404ULL,
    0x1010508481900408ULL,
    0x8000882000040480ULL,
    0x14022400082041ULL,
    0x2062008020020201ULL,
    0x842040100c0100ULL,
    0x81000c04c0b01ULL,
    0x11080100023100ULL,
    0x8c894a100020102ULL,
    0x4001046005082000ULL,
    0x20230048a0212ULL,
    0x3c040c0404000200ULL,
    0x29002011008810ULL,
    0x880100409400ULL,
    0xcc0010400200904ULL,
    0x210040d0022a0ULL,
    0x884490201342201ULL,
    0x4002209008083080ULL,
    0x1100241c0a081008ULL,
    0x500008400881002ULL,
    0x150010c2160000ULL,
    0x2120809042020083ULL,
    0x1002091050008001ULL,
    0x2008031002021078ULL,
    0x220080d01013802ULL,
    0x72202084402088aULL,
    0x8110008201012000ULL,
    0x8400888048441000ULL,
    0x3008080400842402ULL,
    0x800c009004105400ULL,
    0x80002820080230ULL,
    0x40050a2008008101ULL,
    0x9908500410440020ULL,
};

const int RBits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12};

const int BBits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6};
// Transform occupancy to index using magic
static inline int transform(Bitboard occupancy, Bitboard magic, int bits)
{
    return (int)((occupancy * magic) >> (64 - bits));
}

// generate rook attacks for a given square and blockers at initialization
Bitboard generateRookAttacks(int square, Bitboard blockers)
{
    Bitboard attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    // north direction check
    for (int r = rank + 1; r <= 7; r++)
    {
        attacks |= (1ULL << (file + r * 8));
        if (blockers & (1ULL << (file + r * 8)))
        {
            break;
        }
    }

    // south direction check
    for (int r = rank - 1; r >= 0; r--)
    {
        attacks |= (1ULL << (file + r * 8));
        if (blockers & (1ULL << (file + r * 8)))
        {
            break;
        }
    }

    // east direction check
    for (int f = file + 1; f <= 7; f++)
    {
        attacks |= (1ULL << (f + rank * 8));
        if (blockers & (1ULL << (f + rank * 8)))
        {
            break;
        }
    }
    // west direction check
    for (int f = file - 1; f >= 0; f--)
    {
        attacks |= (1ULL << (f + rank * 8));
        if (blockers & (1ULL << (f + rank * 8)))
        {
            break;
        }
    }
    return attacks;
}

// generate bishop attacks for a given square and blockers at initialization
Bitboard generateBishopAttacks(int square, Bitboard blockers)
{
    Bitboard attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    // NE
    for (int r = rank + 1, f = file + 1; r <= 7 && f <= 7; r++, f++)
    {
        attacks |= (1ULL << (r * 8 + f));
        if (blockers & (1ULL << (r * 8 + f)))
            break;
    }
    // NW
    for (int r = rank + 1, f = file - 1; r <= 7 && f >= 0; r++, f--)
    {
        attacks |= (1ULL << (r * 8 + f));
        if (blockers & (1ULL << (r * 8 + f)))
            break;
    }
    // SE
    for (int r = rank - 1, f = file + 1; r >= 0 && f <= 7; r--, f++)
    {
        attacks |= (1ULL << (r * 8 + f));
        if (blockers & (1ULL << (r * 8 + f)))
            break;
    }
    // SW
    for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--)
    {
        attacks |= (1ULL << (r * 8 + f));
        if (blockers & (1ULL << (r * 8 + f)))
            break;
    }

    return attacks;
}

// Initialize all attack tables for rooks and bishops
void initSlidingAttacks(void)
{
    // Initialize rook attacks
    for (int square = 0; square < 64; square++)
    {
        Bitboard mask = rook_occupancy_maps[square];
        int bits = RBits[square];
        int permutations = 1 << bits; // Use bits, not popCount(mask)

        for (int i = 0; i < permutations; i++)
        {
            Bitboard occupancy = 0ULL;
            Bitboard temp_mask = mask;
            int count = 0;

            // Generate occupancy pattern from index
            while (temp_mask)
            {
                int bit_pos = bitboardLSBIndex(temp_mask);
                if (i & (1 << count))
                {
                    occupancy |= (1ULL << bit_pos);
                }
                temp_mask &= temp_mask - 1;
                count++;
            }

            int magic_index = transform(occupancy, RMagic[square], bits);
            rook_attacks[square][magic_index] = generateRookAttacks(square, occupancy);
        }
    }

    // Initialize bishop attacks
    for (int square = 0; square < 64; square++)
    {
        Bitboard mask = bishop_occupancy_maps[square];
        int bits = BBits[square];
        int permutations = 1 << bits; // Use bits, not popCount(mask)

        for (int i = 0; i < permutations; i++)
        {
            Bitboard occupancy = 0ULL;
            Bitboard temp_mask = mask;
            int count = 0;

            // Generate occupancy pattern from index
            while (temp_mask)
            {
                int bit_pos = bitboardLSBIndex(temp_mask);
                if (i & (1 << count))
                {
                    occupancy |= (1ULL << bit_pos);
                }
                temp_mask &= temp_mask - 1;
                count++;
            }

            int magic_index = transform(occupancy, BMagic[square], bits);
            bishop_attacks[square][magic_index] = generateBishopAttacks(square, occupancy);
        }
    }
}

// Lookup rook attacks
Bitboard getRookAttacks(int square, Bitboard occupancy)
{
    occupancy &= rook_occupancy_maps[square];
    int index = transform(occupancy, RMagic[square], RBits[square]);
    return rook_attacks[square][index];
}

// Lookup bishop attacks
Bitboard getBishopAttacks(int square, Bitboard occupancy)
{
    occupancy &= bishop_occupancy_maps[square];
    int index = transform(occupancy, BMagic[square], BBits[square]);
    return bishop_attacks[square][index];
}

// Queen attacks (combination of rook and bishop)
Bitboard getQueenAttacks(int square, Bitboard occupancy)
{
    return getRookAttacks(square, occupancy) | getBishopAttacks(square, occupancy);
}