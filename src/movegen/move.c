#include "movegen/move.h"

Move createMove(Square from, Square to, MoveType type, PieceType promoPiece)
{
    if (from >= NO_SQUARE || to >= NO_SQUARE) {
        return MOVE_NONE;
    }

    Move move = (Move)(((from & 0x3F) << 6) | (to & 0x3F) | type);

    if (type == PROMO) {
        uint16_t promoBits = 0;
        if (promoPiece >= KNIGHT && promoPiece <= QUEEN) {
            promoBits = (uint16_t)(promoPiece - KNIGHT);
        }
        move = (Move)(move | (Move)(promoBits << 12));
    }

    return move;
}

Square getFromSquare(Move move)
{
    if (move == MOVE_NONE)
        return NO_SQUARE;
    return (Square)((move >> 6) & 0x3F);
}

Square getToSquare(Move move)
{
    if (move == MOVE_NONE)
        return NO_SQUARE;
    return (Square)(move & 0x3F);
}

MoveType getMoveType(Move move)
{
    if (move == MOVE_NONE)
        return NORMAL;
    return (MoveType)(move & 0xC000);
}

PieceType getPromotionPieceType(Move move)
{
    if (getMoveType(move) != PROMO)
        return NO_PIECE;
    return (PieceType)(((move >> 12) & 0x3) + KNIGHT);
}

bool move_is_enpassant(Move move)
{
    return (getMoveType(move) == EN_PASSANT);
}

bool move_is_promotion(Move move)
{
    return (getMoveType(move) == PROMO);
}

bool move_is_castling(Move move)
{
    return (getMoveType(move) == CASTLE);
}

bool move_is_capture(CBoard* board, Move move)
{
    if (move == MOVE_NONE)
        return false;

    Square to = getToSquare(move);
    if (board->sideToMove == WHITE) {
        return bitboard_is_bit_set(board->blackPieces, to) && !move_is_enpassant(move) && !move_is_castling(move) && !move_is_promotion(move);
    } else {
        return bitboard_is_bit_set(board->whitePieces, to) && !move_is_enpassant(move) && !move_is_castling(move) && !move_is_promotion(move);
    }
}

bool move_is_quiet(CBoard* board, Move move)
{
    return !move_is_capture(board, move) && !move_is_enpassant(move) && !move_is_castling(move) && !move_is_promotion(move);
}