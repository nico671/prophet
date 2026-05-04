#include <stdbool.h>

#include "cboard.h"
#include "zobrist.h"

uint64_t piece_keys[12][64];
uint64_t side_key;
uint64_t castle_keys[16];
uint64_t en_passant_keys[8];
static bool zobrist_keys_initialized = false;

void init_zobrist_keys(void)
{
    if (zobrist_keys_initialized)
        return;
    zobrist_keys_initialized = true;

    ranctx ctx;
    raninit(&ctx,
        107035250ULL); // Use a fixed seed for reproducible debug sessions

    // 1. Fill Piece Keys
    for (int p = 0; p < 12; p++) {
        for (int s = 0; s < 64; s++) {
            piece_keys[p][s] = ranval(&ctx);
        }
    }

    // 2. Side Key
    side_key = ranval(&ctx);

    // 3. Castling Keys
    for (int i = 0; i < 16; i++) {
        castle_keys[i] = ranval(&ctx);
    }

    // 4. En Passant Keys
    for (int i = 0; i < 8; i++) {
        en_passant_keys[i] = ranval(&ctx);
    }
}

static bool should_hash_ep(const CBoard* board, Square ep_square)
{
    if (!board || ep_square == NO_SQUARE)
        return false;

    if ((unsigned)ep_square >= 64U)
        return false;

    int ep_file = ep_square % 8;

    // EP target must be on rank 6 for white to capture, rank 3 for black to
    // capture.
    if (board->side_to_move == WHITE) {
        if (ep_square < A6 || ep_square > H6)
            return false;

        if (ep_file > 0 && bitboard_is_bit_set(board->white_pawns_bb, ep_square - 9))
            return true;
        if (ep_file < 7 && bitboard_is_bit_set(board->white_pawns_bb, ep_square - 7))
            return true;
        return false;
    }

    if (ep_square < A3 || ep_square > H3)
        return false;

    if (ep_file > 0 && bitboard_is_bit_set(board->black_pawns_bb, ep_square + 7))
        return true;
    if (ep_file < 7 && bitboard_is_bit_set(board->black_pawns_bb, ep_square + 9))
        return true;
    return false;
}

void zobrist_toggle_ep(uint64_t* key, const CBoard* board,
    Square ep_square)
{
    if (should_hash_ep(board, ep_square)) {
        int file = ep_square % 8;
        *key ^= en_passant_keys[file];
    }
}

void compute_zobrist_key(CBoard* board)
{
    // Safety: make initialization impossible to forget.
    init_zobrist_keys();

    uint64_t key = 0ULL;

    // Map your bitboards to an array of pointers to avoid struct layout issues
    uint64_t* piece_bbs[12] = {
        &board->white_pawns_bb, &board->white_knights_bb, &board->white_bishops_bb,
        &board->white_rooks_bb, &board->white_queens_bb, &board->white_king_bb,
        &board->black_pawns_bb, &board->black_knights_bb, &board->black_bishops_bb,
        &board->black_rooks_bb, &board->black_queens_bb, &board->black_king_bb
    };

    for (int piece_type = 0; piece_type < 12; piece_type++) {
        // Copy the bitboard value into a local variable 'bb'
        // Changes to 'bb' do NOT affect board->whitePawns, etc.
        uint64_t bb = *piece_bbs[piece_type];

        while (bb) {
            // Find the index of the least significant bit (0-63)
            int sq = __builtin_ctzll(bb);

            // XOR the random key for this piece/square combo
            key ^= piece_keys[piece_type][sq];

            // Mathematically clear the bit we just processed in the LOCAL copy
            bb &= (bb - 1);
        }
    }

    // 2. Side to move
    if (board->side_to_move == BLACK) {
        key ^= side_key;
    }

    // 3. Castling rights
    key ^= castle_keys[board->castling_rights & 0x0F];

    // 4. En Passant
    zobrist_toggle_ep(&key, board, board->ep_square);

    board->zobrist_key = key;
}
