#include "movegen/move.h"

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

bool move_is_capture(CBoard* board, Move move)
{
    if (move == MOVE_NONE)
        return false;

    Square to = move_get_to_square(move);
    return bitboard_is_bit_set(board->occupancy_bbs[1 - board->side_to_move], to)
        && !move_is_enpassant(move) && !move_is_castling(move)
        && !move_is_promotion(move);
}

bool move_is_quiet(CBoard* board, Move move)
{
    return !move_is_capture(board, move) && !move_is_enpassant(move)
        && !move_is_castling(move) && !move_is_promotion(move);
}