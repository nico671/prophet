#include "zobrist.h"

void initZobristKeys()
{
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

uint64_t computeZobristKey(CBoard *board)
{
    uint64_t key = 0ULL;

    // 1. Pieces
    for (int sq = 0; sq < 64; sq++)
    {
        Bitboard squareMask = (Bitboard)1 << sq;

        // White pieces
        if (board->whitePawns & squareMask)
            key ^= piece_keys[0][sq];
        if (board->whiteKnights & squareMask)
            key ^= piece_keys[1][sq];
        if (board->whiteBishops & squareMask)
            key ^= piece_keys[2][sq];
        if (board->whiteRooks & squareMask)
            key ^= piece_keys[3][sq];
        if (board->whiteQueens & squareMask)
            key ^= piece_keys[4][sq];
        if (board->whiteKing & squareMask)
            key ^= piece_keys[5][sq];

        // Black pieces
        if (board->blackPawns & squareMask)
            key ^= piece_keys[6][sq];
        if (board->blackKnights & squareMask)
            key ^= piece_keys[7][sq];
        if (board->blackBishops & squareMask)
            key ^= piece_keys[8][sq];
        if (board->blackRooks & squareMask)
            key ^= piece_keys[9][sq];
        if (board->blackQueens & squareMask)
            key ^= piece_keys[10][sq];
        if (board->blackKing & squareMask)
            key ^= piece_keys[11][sq];
    }

    // 2. Side to move
    if (board->sideToMove == BLACK)
    {
        key ^= side_key;
    }

    // 3. Castling rights
    int castleIndex = 0;
    if (CHECK_BIT(board->castlingRights, 3))
        castleIndex |= 1;
    if (CHECK_BIT(board->castlingRights, 2))
        castleIndex |= 2;
    if (CHECK_BIT(board->castlingRights, 1))
        castleIndex |= 4;
    if (CHECK_BIT(board->castlingRights, 0))
        castleIndex |= 8;

    key ^= castle_keys[castleIndex];

    // 4. En Passant
    if (board->epSquare != NO_SQUARE)
    {
        int file = board->epSquare % 8;
        key ^= en_passant_keys[file];
    }
    return key;
}