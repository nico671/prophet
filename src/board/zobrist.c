#include "zobrist.h"
#include "board/cboard.h"

#include <stdbool.h>

uint64_t piece_keys[12][64];
uint64_t side_key;
uint64_t castle_keys[16];
uint64_t en_passant_keys[8];
static bool zobristKeysInitialized = false;

void initZobristKeys()
{
    if (zobristKeysInitialized)
        return;
    zobristKeysInitialized = true;

    ranctx ctx;
    raninit(&ctx, 107035250ULL); // Use a fixed seed for reproducible debug sessions

    // 1. Fill Piece Keys
    for (int p = 0; p < 12; p++)
    {
        for (int s = 0; s < 64; s++)
        {
            piece_keys[p][s] = ranval(&ctx);
        }
    }

    // 2. Side Key
    side_key = ranval(&ctx);

    // 3. Castling Keys
    for (int i = 0; i < 16; i++)
    {
        castle_keys[i] = ranval(&ctx);
    }

    // 4. En Passant Keys
    for (int i = 0; i < 8; i++)
    {
        en_passant_keys[i] = ranval(&ctx);
    }
}

static bool shouldHashEnPassant(const CBoard *board, Square epSquare)
{
    if (!board || epSquare == NO_SQUARE)
        return false;

    if ((unsigned)epSquare >= 64U)
        return false;

    int epFile = epSquare % 8;

    // EP target must be on rank 6 for white to capture, rank 3 for black to capture.
    if (board->sideToMove == WHITE)
    {
        if (epSquare < A6 || epSquare > H6)
            return false;

        if (epFile > 0 && bitboardIsBitSet(board->whitePawns, epSquare - 9))
            return true;
        if (epFile < 7 && bitboardIsBitSet(board->whitePawns, epSquare - 7))
            return true;
        return false;
    }

    if (epSquare < A3 || epSquare > H3)
        return false;

    if (epFile > 0 && bitboardIsBitSet(board->blackPawns, epSquare + 7))
        return true;
    if (epFile < 7 && bitboardIsBitSet(board->blackPawns, epSquare + 9))
        return true;
    return false;
}

void zobristToggleEnPassant(uint64_t *key, const CBoard *board, Square epSquare)
{
    if (shouldHashEnPassant(board, epSquare))
    {
        int file = epSquare % 8;
        *key ^= en_passant_keys[file];
    }
}

void computeZobristKey(CBoard *board)
{
    // Safety: make initialization impossible to forget.
    initZobristKeys();

    uint64_t key = 0ULL;

    // Map your bitboards to an array of pointers to avoid struct layout issues
    uint64_t *pieceBBs[12] = {
        &board->whitePawns, &board->whiteKnights, &board->whiteBishops,
        &board->whiteRooks, &board->whiteQueens, &board->whiteKing,
        &board->blackPawns, &board->blackKnights, &board->blackBishops,
        &board->blackRooks, &board->blackQueens, &board->blackKing};

    for (int pieceType = 0; pieceType < 12; pieceType++)
    {
        // Copy the bitboard value into a local variable 'bb'
        // Changes to 'bb' do NOT affect board->whitePawns, etc.
        uint64_t bb = *pieceBBs[pieceType];

        while (bb)
        {
            // Find the index of the least significant bit (0-63)
            int sq = __builtin_ctzll(bb);

            // XOR the random key for this piece/square combo
            key ^= piece_keys[pieceType][sq];

            // Mathematically clear the bit we just processed in the LOCAL copy
            bb &= (bb - 1);
        }
    }

    // 2. Side to move
    if (board->sideToMove == BLACK)
    {
        key ^= side_key;
    }

    // 3. Castling rights
    key ^= castle_keys[board->castlingRights & 0x0F];

    // 4. En Passant
    zobristToggleEnPassant(&key, board, board->epSquare);

    board->zobristKey = key;
}