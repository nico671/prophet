#include "attacks/sliding_attacks.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define ROOK_TABLE_SIZE 4096
#define BISHOP_TABLE_SIZE 512
#define ROOK_TABLE_BITS 12
#define BISHOP_TABLE_BITS 9

// ensure that our table sizes match the max bits we need to index them
_Static_assert(ROOK_TABLE_SIZE == (1 << ROOK_TABLE_BITS), "rook table size must match max rook bits");
_Static_assert(BISHOP_TABLE_SIZE == (1 << BISHOP_TABLE_BITS), "bishop table size must match max bishop bits");

// maps a masked occupancy bitboard to a compact index into the attack table for that square.
static inline int magicIndex(Bitboard occupancy, Bitboard magic, int bits)
{
    return (int)((occupancy * magic) >> (64 - bits));
}

// Materialized attack tables (single definition)
// used for fast lookup in move generation and evaluation
Bitboard rook_attacks[64][4096];
Bitboard bishop_attacks[64][512];

// *_occupancy_maps are masks of relevant blocker squares for each origin square
// they are used to generate the occupancy variations for testing magic candidates and generating the attack tables
static const Bitboard rook_occupancy_maps[64] = {
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

static const Bitboard bishop_occupancy_maps[64] = {
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

// magic multipliers per square used to index into the attack tables after masking blockers and multiplying by the magic multiplier, then shifting to get the final index
static const Bitboard RMagic[64] = {
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

static const Bitboard BMagic[64] = {
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

// relevant number of bits per square used for indexing into the attack tables after masking blockers and multiplying by the magic multiplier, then shifting to get the final index
static const int RBits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

static const int BBits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

typedef Bitboard (*AttackGenerator)(Square square, Bitboard blockers);

// enumerates all relevant blocker bit positions for a given square and stores them in the provided positions array, returning the count of positions found. used to generate the occupancy variations for testing magic candidates and generating the attack tables.
static int extractMaskBitPositions(Bitboard mask, int* positions, int maxPositions)
{
    int count = 0;
    while (mask) {
        if (count >= maxPositions) {
            assert(count < maxPositions);
            break;
        }
        positions[count++] = bitboard_lsb_index(mask);
        mask &= mask - 1;
    }
    return count;
}

// converts permutation index to occupancy bitboard
//
static Bitboard occupancyFromIndex(int occupancyIndex, const int* positions, int relevantBits)
{
    Bitboard occupancy = 0ULL;
    for (int bit = 0; bit < relevantBits; ++bit) {
        if (occupancyIndex & (1 << bit)) {
            bitboard_set_square_bit(&occupancy, positions[bit]);
        }
    }
    return occupancy;
}

// generic attack table initializer for sliding pieces, used to generate both rook and bishop tables
static void initializeAttackTable(
    Bitboard* table,
    int tableSize,
    const Bitboard occupancyMaps[64],
    const Bitboard magics[64],
    const int bitsPerSquare[64],
    AttackGenerator attackGenerator)
{
    // collision detection checker
    bool used[ROOK_TABLE_SIZE];
    // loop all squares
    for (int square = 0; square < 64; ++square) {
        const int bits = bitsPerSquare[square];
        const int permutations = 1 << bits;
        int bitPositions[ROOK_TABLE_BITS] = { 0 };
        const int extractedBits = extractMaskBitPositions(occupancyMaps[square], bitPositions, ROOK_TABLE_BITS);

        assert(bits <= ROOK_TABLE_BITS);
        assert(permutations <= tableSize);
        assert(extractedBits == bits);
        if (bits > ROOK_TABLE_BITS || permutations > tableSize || extractedBits != bits) {
            abort();
        }

        memset(used, 0, (size_t)tableSize * sizeof(used[0]));

        for (int i = 0; i < permutations; ++i) {
            const Bitboard occupancy = occupancyFromIndex(i, bitPositions, bits);
            const int magic_index = magicIndex(occupancy, magics[square], bits);
            const Bitboard attacks = attackGenerator(square, occupancy);

            assert(magic_index >= 0 && magic_index < tableSize);

            if (used[magic_index]) {
                assert(table[square * tableSize + magic_index] == attacks);
                continue;
            }

            used[magic_index] = true;
            table[square * tableSize + magic_index] = attacks;
        }
    }
}

// generate rook attacks for a given square and blockers at initialization
Bitboard generateRookAttacks(Square square, Bitboard blockers)
{
    Bitboard attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    // north direction check
    for (int r = rank + 1; r <= 7; r++) {
        bitboard_set_square_bit(&attacks, file + r * 8);
        if (bitboard_is_bit_set(blockers, file + r * 8)) {
            break;
        }
    }

    // south direction check
    for (int r = rank - 1; r >= 0; r--) {
        bitboard_set_square_bit(&attacks, file + r * 8);
        if (bitboard_is_bit_set(blockers, file + r * 8)) {
            break;
        }
    }

    // east direction check
    for (int f = file + 1; f <= 7; f++) {
        bitboard_set_square_bit(&attacks, f + rank * 8);
        if (bitboard_is_bit_set(blockers, f + rank * 8)) {
            break;
        }
    }
    // west direction check
    for (int f = file - 1; f >= 0; f--) {
        bitboard_set_square_bit(&attacks, f + rank * 8);
        if (bitboard_is_bit_set(blockers, f + rank * 8)) {
            break;
        }
    }
    return attacks;
}

// generate bishop attacks for a given square and blockers at initialization
Bitboard generateBishopAttacks(Square square, Bitboard blockers)
{
    Bitboard attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    // NE
    for (int r = rank + 1, f = file + 1; r <= 7 && f <= 7; r++, f++) {
        bitboard_set_square_bit(&attacks, r * 8 + f);
        if (bitboard_is_bit_set(blockers, r * 8 + f))
            break;
    }
    // NW
    for (int r = rank + 1, f = file - 1; r <= 7 && f >= 0; r++, f--) {
        bitboard_set_square_bit(&attacks, r * 8 + f);
        if (bitboard_is_bit_set(blockers, r * 8 + f))
            break;
    }
    // SE
    for (int r = rank - 1, f = file + 1; r >= 0 && f <= 7; r--, f++) {
        bitboard_set_square_bit(&attacks, r * 8 + f);
        if (bitboard_is_bit_set(blockers, r * 8 + f))
            break;
    }
    // SW
    for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--) {
        bitboard_set_square_bit(&attacks, r * 8 + f);
        if (bitboard_is_bit_set(blockers, r * 8 + f))
            break;
    }

    return attacks;
}

Bitboard getRookAttacks(Square square, Bitboard occupancy)
{
    occupancy &= rook_occupancy_maps[square];
    int index = magicIndex(occupancy, RMagic[square], RBits[square]);
    return rook_attacks[square][index];
}

Bitboard getBishopAttacks(Square square, Bitboard occupancy)
{
    occupancy &= bishop_occupancy_maps[square];
    int index = magicIndex(occupancy, BMagic[square], BBits[square]);
    return bishop_attacks[square][index];
}

Bitboard getQueenAttacks(Square square, Bitboard occupancy)
{
    return getRookAttacks(square, occupancy) | getBishopAttacks(square, occupancy);
}

void initSlidingAttacks(void)
{
    static atomic_int initState = 0; // 0=uninitialized, 1=initializing, 2=initialized
    int expected = 0;

    if (atomic_compare_exchange_strong_explicit(
            &initState, &expected, 1, memory_order_acq_rel, memory_order_acquire)) {
        initializeAttackTable(&rook_attacks[0][0],
            ROOK_TABLE_SIZE,
            rook_occupancy_maps,
            RMagic,
            RBits,
            generateRookAttacks);

        initializeAttackTable(&bishop_attacks[0][0],
            BISHOP_TABLE_SIZE,
            bishop_occupancy_maps,
            BMagic,
            BBits,
            generateBishopAttacks);

        atomic_store_explicit(&initState, 2, memory_order_release);
        return;
    }

    while (atomic_load_explicit(&initState, memory_order_acquire) != 2) {
    }
}