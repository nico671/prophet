#ifndef PROPHET_MOVE_H
#define PROPHET_MOVE_H

#include <stdbool.h>
#include <stdint.h>
#include "core/chess_types.h"

// Move flag encoding.
// typedef enum
// {
// QUIET = 0,
// DOUBLE_PAWN_PUSH = 1,
// KINGSIDE_CASTLE = 2,
// QUEENSIDE_CASTLE = 3,
// CAPTURE = 4,
// EP_CAPTURE = 5,

//     KNIGHT_PROMO_QUIET = 8,
//     BISHOP_PROMO_QUIET = 9,
//     ROOK_PROMO_QUIET = 10,
//     QUEEN_PROMO_QUIET = 11,

//     KNIGHT_PROMO_CAPTURE = 12,
//     BISHOP_PROMO_CAPTURE = 13,
//     ROOK_PROMO_CAPTURE = 14,
//     QUEEN_PROMO_CAPTURE = 15,
// } MoveFlag;

// typedef struct
// {
//     Square from;
//     Square to;
//     MoveFlag flag;
// } Move;

/**
 * @brief Move representation using a compact 16-bit encoding. The move is represented as a uint16_t where:
 * - Bits 0-5 (6 bits): To square index (0-63 corresponding to A1-H8)
 * - Bits 6-11 (6 bits): From square index (0-63 corresponding to A1-H8)
 * - Bits 12-13 (2 bits): Promotion piece type for promotion moves (00=Knight, 01=Bishop, 10=Rook, 11=Queen)
 * - Bits 14-15 (2 bits): Move type flags (00=NORMAL, 01=Promotion, 10=En Passant, 11=Castling) note that en passant is only present when a pawn can be captured en passant
 */
typedef uint16_t Move;
#define MOVE_NONE ((Move)0xFFFF)
// TODO: use stockfish move representation
// https://github.com/official-stockfish/Stockfish/blob/master/src/types.h#L428

// Move list structure
typedef struct
{
    Move moves[256]; // Maximum possible moves in a position
    int count;
} MoveList;

typedef enum
{
    NORMAL = 0,
    PROMO = 1u << 14,
    EN_PASSANT = 2u << 14,
    CASTLE = 3u << 14
} MoveType;

/**
 * @brief Creates a move with the specified parameters.
 *
 * @param from The starting square of the move.
 * @param to The destination square of the move.
 * @param type The type of the move.
 * @param promoPiece The piece type for promotion moves.
 * @return Move The created move.
 */
Move createMove(Square from, Square to, MoveType type, PieceType promoPiece);
Square getFromSquare(Move move);

Square getToSquare(Move move);

MoveType getMoveType(Move move);

PieceType getPromotionPieceType(Move move);
bool move_is_enpassant(Move move);
bool move_is_promotion(Move move);
bool move_is_castling(Move move);

// // --- Move construction helpers ---
// #define MAKE_MOVE(from, to) ((Move){(from), (to), QUIET})
// #define MAKE_CAPTURE(from, to) ((Move){(from), (to), CAPTURE})
// #define MAKE_EP(from, to) ((Move){(from), (to), EP_CAPTURE})
// #define MAKE_DOUBLE_PUSH(from, to) ((Move){(from), (to), DOUBLE_PAWN_PUSH})
// #define MAKE_CASTLE_KING(from, to) ((Move){(from), (to), KINGSIDE_CASTLE})
// #define MAKE_CASTLE_QUEEN(from, to) ((Move){(from), (to), QUEENSIDE_CASTLE})

// static inline Move make_promotion_move(Square from, Square to, PieceType pieceType, bool isCapture)
// {
//     MoveFlag promoFlag;
//     int offset = isCapture ? 4 : 0;
//     switch (pieceType)
//     {
//     case KNIGHT:
//         promoFlag = (MoveFlag)(KNIGHT_PROMO_QUIET + offset);
//         break;
//     case BISHOP:
//         promoFlag = (MoveFlag)(BISHOP_PROMO_QUIET + offset);
//         break;
//     case ROOK:
//         promoFlag = (MoveFlag)(ROOK_PROMO_QUIET + offset);
//         break;
//     case QUEEN:
//         promoFlag = (MoveFlag)(QUEEN_PROMO_QUIET + offset);
//         break;
//     default:
//         promoFlag = QUIET;
//         break;
//     }
//     return (Move){from, to, promoFlag};
// }

// // Backwards-compatible name used throughout current code.
// #define MAKE_PROMOTION(from, to, pieceType, isCapture) make_promotion_move((from), (to), (pieceType), (isCapture))

// #define FROM_SQ(move) ((move).from)
// #define TO_SQ(move) ((move).to)
// #define MOVE_FLAG(move) ((move).flag)

// static inline PieceType getPromotionPieceType(Move move)
// {
//     switch (move.flag)
//     {
//     case KNIGHT_PROMO_QUIET:
//     case KNIGHT_PROMO_CAPTURE:
//         return KNIGHT;
//     case BISHOP_PROMO_QUIET:
//     case BISHOP_PROMO_CAPTURE:
//         return BISHOP;
//     case ROOK_PROMO_QUIET:
//     case ROOK_PROMO_CAPTURE:
//         return ROOK;
//     case QUEEN_PROMO_QUIET:
//     case QUEEN_PROMO_CAPTURE:
//         return QUEEN;
//     default:
//         return NO_PIECE;
//     }
// }

// static inline bool move_is_capture(Move move)
// {
//     return (move.flag == CAPTURE || move.flag == EP_CAPTURE ||
//             move.flag == KNIGHT_PROMO_CAPTURE || move.flag == BISHOP_PROMO_CAPTURE ||
//             move.flag == ROOK_PROMO_CAPTURE || move.flag == QUEEN_PROMO_CAPTURE);
// }

// static inline bool move_is_promotion(Move move)
// {
//     return (move.flag == KNIGHT_PROMO_QUIET || move.flag == BISHOP_PROMO_QUIET ||
//             move.flag == ROOK_PROMO_QUIET || move.flag == QUEEN_PROMO_QUIET ||
//             move.flag == KNIGHT_PROMO_CAPTURE || move.flag == BISHOP_PROMO_CAPTURE ||
//             move.flag == ROOK_PROMO_CAPTURE || move.flag == QUEEN_PROMO_CAPTURE);
// }

// static inline bool move_is_en_passant(Move move)
// {
//     return (move.flag == EP_CAPTURE);
// }

// static inline bool move_is_castling(Move move)
// {
//     return (move.flag == KINGSIDE_CASTLE || move.flag == QUEENSIDE_CASTLE);
// }

// static inline bool move_is_double_push(Move move)
// {
//     return (move.flag == DOUBLE_PAWN_PUSH);
// }

#endif // PROPHET_MOVE_H
