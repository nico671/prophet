#include "zobrist.h"
#include "board/cboard.h"

#include <stdbool.h>

uint64_t piece_keys[12][64];
uint64_t side_key;
uint64_t castle_keys[16];
uint64_t en_passant_keys[8];

void initZobristKeys()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

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

void computeZobristKey(CBoard *board)
{
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
    // Ensure castlingRights is always 0-15
    key ^= castle_keys[board->castlingRights];

    // 4. En Passant
    if (board->epSquare != NO_SQUARE)
    {
        int file = board->epSquare % 8;
        key ^= en_passant_keys[file];
    }

    board->zobristKey = key;
}