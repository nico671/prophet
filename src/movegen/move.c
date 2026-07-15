#include "movegen/move.h"

#include "movegen/movegen.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void set_error(char* error_buf, size_t error_buf_size, const char* message)
{
    if (!error_buf || error_buf_size == 0) {
        return;
    }
    snprintf(error_buf, error_buf_size, "%s", message);
}

static Square algebraic_notation_to_square(const char* algebraic_square_str)
{
    if (strlen(algebraic_square_str) < 2) {
        return NO_SQUARE;
    }

    char file = algebraic_square_str[0];
    char rank = algebraic_square_str[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
        return NO_SQUARE;
    }

    return (Square)((rank - '1') * 8 + (file - 'a'));
}

Move create_move(Square from, Square to, MoveType type, PieceType promo_piecetype)
{
    if (from >= NO_SQUARE || to >= NO_SQUARE) {
        return MOVE_NONE;
    }

    // thanks stockfish
    Move move = (Move)(((from & 0x3F) << 6) | (to & 0x3F) | type);

    if (type == PROMO) {
        uint16_t promoBits = 0;
        if (promo_piecetype >= KNIGHT && promo_piecetype <= QUEEN) {
            promoBits = (uint16_t)(promo_piecetype - KNIGHT);
        }
        move = (Move)(move | (Move)(promoBits << 12));
    }

    return move;
}

Square move_get_from_square(Move move)
{
    if (move == MOVE_NONE)
        return NO_SQUARE;
    return (Square)((move >> 6) & 0x3F);
}

Square move_get_to_square(Move move)
{
    if (move == MOVE_NONE)
        return NO_SQUARE;
    return (Square)(move & 0x3F);
}

MoveType move_get_move_type(Move move)
{
    if (move == MOVE_NONE)
        return NORMAL;
    return (MoveType)(move & 0xC000);
}

PieceType move_get_promotion_piecetype(Move move)
{
    if (move_get_move_type(move) != PROMO)
        return NO_PIECE;
    return (PieceType)(((move >> 12) & 0x3) + KNIGHT);
}

bool move_is_enpassant(Move move)
{
    return (move_get_move_type(move) == EN_PASSANT);
}

bool move_is_promotion(Move move) { return (move_get_move_type(move) == PROMO); }

bool move_is_castling(Move move) { return (move_get_move_type(move) == CASTLE); }

bool move_is_capture(const CBoard* board, Move move)
{
    if (move == MOVE_NONE)
        return false;

    if (move_is_enpassant(move)) {
        return true;
    }

    Square to = move_get_to_square(move);
    return bitboard_is_bit_set(board->occupancy_bbs[1 - board->side_to_move], to)
           && !move_is_castling(move);
}

bool move_is_quiet(const CBoard* board, Move move)
{
    return !move_is_capture(board, move) && !move_is_castling(move)
           && !move_is_promotion(move);
}

void move_to_uci_string(Move move, char out[6])
{
    if (move_get_from_square(move) == NO_SQUARE
        || move_get_to_square(move) == NO_SQUARE) {
        strcpy(out, "0000");
        return;
    }

    out[0] = (char)('a' + (move_get_from_square(move) % 8));
    out[1] = (char)('1' + (move_get_from_square(move) / 8));
    out[2] = (char)('a' + (move_get_to_square(move) % 8));
    out[3] = (char)('1' + (move_get_to_square(move) / 8));

    if (move_is_promotion(move)) {
        PieceType promo = move_get_promotion_piecetype(move);
        if (promo == KNIGHT) {
            out[4] = 'n';
        } else if (promo == BISHOP) {
            out[4] = 'b';
        } else if (promo == ROOK) {
            out[4] = 'r';
        } else {
            out[4] = 'q';
        }
        out[5] = '\0';
        return;
    }

    out[4] = '\0';
}

Move move_from_uci_string(const CBoard* board, const char* move_str, char* error_buf,
                          size_t error_buf_size)
{
    if (!board || !move_str) {
        set_error(error_buf, error_buf_size, "Invalid move input");
        return MOVE_NONE;
    }

    size_t move_len = strlen(move_str);
    if (move_len < 4 || move_len > 5) {
        set_error(error_buf, error_buf_size, "Invalid move format");
        return MOVE_NONE;
    }

    Square from = algebraic_notation_to_square(move_str);
    Square to   = algebraic_notation_to_square(move_str + 2);
    if (from == NO_SQUARE || to == NO_SQUARE) {
        set_error(error_buf, error_buf_size, "Invalid move format");
        return MOVE_NONE;
    }

    CBoard   board_copy = *board;
    MoveList move_list;
    init_move_list(&move_list);
    generate_legal_moves(&board_copy, &move_list);

    char promotion_char = move_len == 5 ? (char)tolower((unsigned char)move_str[4])
                                        : '\0';

    for (int i = 0; i < move_list.count; i++) {
        Move move = move_list.moves[i];
        if (move_get_from_square(move) != from || move_get_to_square(move) != to) {
            continue;
        }
        if (promotion_char == '\0') {
            return move;
        }
        if (!move_is_promotion(move)) {
            continue;
        }

        PieceType promo = move_get_promotion_piecetype(move);
        if ((promotion_char == 'n' && promo == KNIGHT)
            || (promotion_char == 'b' && promo == BISHOP)
            || (promotion_char == 'r' && promo == ROOK)
            || (promotion_char == 'q' && promo == QUEEN)) {
            return move;
        }
    }

    set_error(error_buf, error_buf_size, "Move not in legal moves list");
    return MOVE_NONE;
}
