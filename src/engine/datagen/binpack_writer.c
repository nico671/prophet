/*
 * Minimal writer for nnue-pytorch's binpack format, adapted from
 * https://github.com/official-stockfish/nnue-pytorch at
 * 86adb4b075456144d58a69398303ab9ce9ec6590 (GPL-3.0-only).
 */
#include "engine/datagen/binpack_writer.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { CHUNK_SIZE = 1024 * 1024, RECORD_SIZE = 34 };

static uint16_t signed_to_unsigned(int16_t value)
{
    uint16_t bits;
    memcpy(&bits, &value, sizeof(bits));
    if (bits & 0x8000U) {
        bits ^= 0x7fffU;
    }
    return (uint16_t)((bits << 1) | (bits >> 15));
}

static bool flush(BinpackWriter* writer)
{
    if (writer->used == 0) {
        return true;
    }
    FILE* file              = writer->file;
    unsigned char header[8] = { 'B',
                                'I',
                                'N',
                                'P',
                                (unsigned char)writer->used,
                                (unsigned char)(writer->used >> 8),
                                (unsigned char)(writer->used >> 16),
                                (unsigned char)(writer->used >> 24) };
    if (fwrite(header, 1, sizeof(header), file) != sizeof(header)
        || fwrite(writer->buffer, 1, writer->used, file) != writer->used) {
        return false;
    }
    writer->used = 0;
    return true;
}

static int piece_nibble(const CBoard* board, Square square, Color color, PieceType piece)
{
    if (piece == PAWN && board->ep_square != NO_SQUARE && square % 8 == board->ep_square % 8
        && ((square / 8 == 3 && board->side_to_move == BLACK)
            || (square / 8 == 4 && board->side_to_move == WHITE))) {
        return 12;
    }
    if (piece == ROOK
        && ((color == WHITE
             && ((square == A1 && U8_CHECK_BIT(board->castling_rights, 2))
                 || (square == H1 && U8_CHECK_BIT(board->castling_rights, 3))))
            || (color == BLACK
                && ((square == A8 && U8_CHECK_BIT(board->castling_rights, 0))
                    || (square == H8 && U8_CHECK_BIT(board->castling_rights, 1)))))) {
        return color == WHITE ? 13 : 14;
    }
    if (piece == KING) {
        return color == WHITE ? 10 : board->side_to_move == WHITE ? 11 : 15;
    }
    return (piece - 1) * 2 + color;
}

static bool pack_position(unsigned char out[24], const CBoard* board)
{
    uint64_t occupancy = board->occupancy_bbs[2];
    for (int i = 0; i < 8; i++) {
        out[i] = (unsigned char)(occupancy >> (56 - 8 * i));
    }
    memset(out + 8, 0, 16);
    int index = 0;
    for (Square square = A1; square < NO_SQUARE; square++) {
        if (!bitboard_is_bit_set(occupancy, square)) {
            continue;
        }
        PieceType piece = cboard_get_piece_at_square(board, square);
        Color color     = bitboard_is_bit_set(board->occupancy_bbs[WHITE], square) ? WHITE : BLACK;
        if (piece == NO_PIECE || index >= 32) {
            return false;
        }
        int nibble = piece_nibble(board, square, color, piece);
        out[8 + index / 2] |= (unsigned char)(nibble << ((index & 1) * 4));
        index++;
    }
    return true;
}

static bool pack_move(unsigned char out[2], Move move)
{
    if (move == MOVE_NONE) {
        return false;
    }
    Square from          = move_get_from_square(move);
    Square to            = move_get_to_square(move);
    MoveType type        = move_get_move_type(move);
    unsigned format_type = 0;
    if (type == PROMO) {
        format_type = 1;
    } else if (type == CASTLE) {
        format_type = 2;
        to          = to > from ? (from == E1 ? H1 : H8) : (from == E1 ? A1 : A8);
    } else if (type == EN_PASSANT) {
        format_type = 3;
    } else if (type != NORMAL) {
        return false;
    }
    unsigned promotion = type == PROMO ? (unsigned)move_get_promotion_piecetype(move) - KNIGHT : 0;
    if (from >= NO_SQUARE || to >= NO_SQUARE || promotion > 3) {
        return false;
    }
    uint16_t packed
        = (uint16_t)((format_type << 14) | ((unsigned)from << 8) | ((unsigned)to << 2) | promotion);
    out[0] = (unsigned char)(packed >> 8);
    out[1] = (unsigned char)packed;
    return true;
}

bool binpack_open(BinpackWriter* writer, const char* path)
{
    *writer        = (BinpackWriter) { 0 };
    writer->file   = fopen(path, "wbx");
    writer->buffer = malloc(CHUNK_SIZE);
    if (!writer->file || !writer->buffer) {
        if (writer->file)
            fclose(writer->file);
        free(writer->buffer);
        *writer = (BinpackWriter) { 0 };
        return false;
    }
    return true;
}

bool binpack_append(BinpackWriter* writer, const CBoard* board, Move move, int score, int ply,
                    double result)
{
    if (!writer->file || !board || score < INT16_MIN || score > INT16_MAX || ply < 0 || ply > 0x3fff
        || (result != 0.0 && result != 0.5 && result != 1.0)) {
        return false;
    }
    if (writer->used + RECORD_SIZE > CHUNK_SIZE && !flush(writer)) {
        return false;
    }
    unsigned char* out = writer->buffer + writer->used;
    if (!pack_position(out, board) || !pack_move(out + 24, move)) {
        return false;
    }
    uint16_t encoded_score  = signed_to_unsigned((int16_t)score);
    int16_t signed_result   = result == 0.5 ? 0 : result == 1.0 ? 1 : -1;
    uint16_t encoded_result = signed_to_unsigned(signed_result);
    uint16_t ply_result     = (uint16_t)ply | (uint16_t)(encoded_result << 14);
    out[26]                 = (unsigned char)(encoded_score >> 8);
    out[27]                 = (unsigned char)encoded_score;
    out[28]                 = (unsigned char)(ply_result >> 8);
    out[29]                 = (unsigned char)ply_result;
    out[30]                 = (unsigned char)(board->half_move_clock >> 8);
    out[31]                 = (unsigned char)board->half_move_clock;
    out[32]                 = 0;
    out[33]                 = 0;
    writer->used += RECORD_SIZE;
    writer->records++;
    return true;
}

bool binpack_close(BinpackWriter* writer)
{
    bool success = writer->file && flush(writer);
    if (writer->file && fclose(writer->file) != 0) {
        success = false;
    }
    free(writer->buffer);
    *writer = (BinpackWriter) { 0 };
    return success;
}
