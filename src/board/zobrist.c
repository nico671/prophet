#include "zobrist.h"

#include "cboard.h"

#include <stdbool.h>

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
            107035250ULL); // Use a fixed seed for reproducible debug
                           // sessions

    // piece keys
    for (int p = 0; p < 12; p++) {
        for (int s = 0; s < 64; s++) {
            piece_keys[p][s] = ranval(&ctx);
        }
    }

    // side key
    side_key = ranval(&ctx);

    // castling rights
    for (int i = 0; i < 16; i++) {
        castle_keys[i] = ranval(&ctx);
    }

    // ep keys
    for (int i = 0; i < 8; i++) {
        en_passant_keys[i] = ranval(&ctx);
    }
}

static bool should_hash_ep(const CBoard* board, Square ep_square)
{
    // ensure inputs valid
    if (!board || (unsigned)ep_square >= 64U) {
        return false;
    }

    int ep_file = ep_square % 8;

    // EP target must be on rank 6 for white to capture since black
    // must move from rank 7 to 5 for ep to be possible, and vice
    // versa for black capturing
    if (board->side_to_move == WHITE) {
        if (ep_square < A6 || ep_square > H6)
            return false;

        if (ep_file > 0
            && bitboard_is_bit_set(board->piece_bbs[WHITE][PAWN], ep_square - 9))
            return true;
        if (ep_file < 7
            && bitboard_is_bit_set(board->piece_bbs[WHITE][PAWN], ep_square - 7))
            return true;
        return false;
    }

    // same logic but with rank 3 for black to capture
    if (ep_square < A3 || ep_square > H3)
        return false;

    if (ep_file > 0
        && bitboard_is_bit_set(board->piece_bbs[BLACK][PAWN], ep_square + 7))
        return true;
    if (ep_file < 7
        && bitboard_is_bit_set(board->piece_bbs[BLACK][PAWN], ep_square + 9))
        return true;
    return false;
}

void zobrist_toggle_ep(uint64_t* key, const CBoard* board, Square ep_square)
{
    if (should_hash_ep(board, ep_square)) {
        int file = ep_square % 8;
        *key ^= en_passant_keys[file];
    }
}

void compute_zobrist_key(CBoard* board)
{
    // kinda pointless?
    // init_zobrist_keys();

    uint64_t key = 0ULL;

    for (int piece_type = 0; piece_type < 12; piece_type++) {
        uint64_t bb = board->piece_bbs[piece_type / 6][piece_type % 6];

        while (bb) {
            Square sq = bitboard_pop_lsb_unsafe(&bb);
            // XOR the random key for this piece/square combo
            key ^= piece_keys[piece_type][sq];
        }
    }

    if (board->side_to_move == BLACK) {
        key ^= side_key;
    }

    key ^= castle_keys[board->castling_rights & 0x0F];

    zobrist_toggle_ep(&key, board, board->ep_square);

    board->zobrist_key = key;
}
