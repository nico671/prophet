
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "core/bitboard.h"
#include "board/cboard.h"
#include "attacks/constant_attacks.h"
#include "attacks/sliding_attacks.h"

// Move flag encoding
// TODO: move away from enum definition, use an int with each bit having a different meaning
typedef enum
{
    QUIET = 0,
    DOUBLE_PAWN_PUSH = 1,
    KINGSIDE_CASTLE = 2,
    QUEENSIDE_CASTLE = 3,
    CAPTURE = 4,
    EP_CAPTURE = 5,
    KNIGHT_PROMO_QUIET = 8,
    BISHOP_PROMO_QUIET = 9,
    ROOK_PROMO_QUIET = 10,
    QUEEN_PROMO_QUIET = 11,
    KNIGHT_PROMO_CAPTURE = 12,
    BISHOP_PROMO_CAPTURE = 13,
    ROOK_PROMO_CAPTURE = 14,
    QUEEN_PROMO_CAPTURE = 15,
} MoveFlag;

typedef struct
{
    Square from;
    Square to;
    MoveFlag flag;
} Move;

// Move list structure
typedef struct
{
    Move moves[256]; // Maximum possible moves in a position
    int count;
} MoveList;

// move construction macros

// Create a quiet move
#define MAKE_MOVE(from, to) ((Move){from, to, QUIET})

// make a capture move
#define MAKE_CAPTURE(from, to) ((Move){from, to, CAPTURE})
// make an en passant move
#define MAKE_EP(from, to) ((Move){from, to, EP_CAPTURE})
// make a double pawn push move
#define MAKE_DOUBLE_PUSH(from, to) ((Move){from, to, DOUBLE_PAWN_PUSH})
// make a kingside castle move
#define MAKE_CASTLE_KING(from, to) ((Move){from, to, KINGSIDE_CASTLE})
// make a queenside castle move
#define MAKE_CASTLE_QUEEN(from, to) ((Move){from, to, QUEENSIDE_CASTLE})

// make a promotion move
static inline Move MAKE_PROMOTION(Square from, Square to, PieceType pieceType, bool isCapture)
{
    MoveFlag promoFlag;
    int offset = isCapture ? 4 : 0;
    switch (pieceType)
    {
    case KNIGHT:
        promoFlag = (MoveFlag)(KNIGHT_PROMO_QUIET + offset);
        break;
    case BISHOP:
        promoFlag = (MoveFlag)(BISHOP_PROMO_QUIET + offset);
        break;
    case ROOK:
        promoFlag = (MoveFlag)(ROOK_PROMO_QUIET + offset);
        break;
    case QUEEN:
        promoFlag = (MoveFlag)(QUEEN_PROMO_QUIET + offset);
        break;
    default:
        promoFlag = QUIET; // default to quiet if invalid pieceType
        break;
    }
    return ((Move){from, to, promoFlag});
}

// make a promotion capture move
static inline Move MAKE_PROMOTION_CAPTURE(Square from, Square to, PieceType pieceType)
{
    MoveFlag promoCapFlag;
    switch (pieceType)
    {
    case KNIGHT:
        promoCapFlag = KNIGHT_PROMO_CAPTURE;
        break;
    case BISHOP:
        promoCapFlag = BISHOP_PROMO_CAPTURE;
        break;
    case ROOK:
        promoCapFlag = ROOK_PROMO_CAPTURE;
        break;
    case QUEEN:
        promoCapFlag = QUEEN_PROMO_CAPTURE;
        break;
    default:
        promoCapFlag = QUIET; // default to quiet if invalid pieceType
        break;
    }
    return ((Move){from, to, promoCapFlag});
}

// make an en passant capture move
static inline Move MAKE_EP_CAPTURE(Square from, Square to)
{
    return ((Move){from, to, EP_CAPTURE});
}

#define FROM_SQ(move) ((move).from)
#define TO_SQ(move) ((move).to)
#define MOVE_FLAG(move) ((move).flag)

static inline int getPromotionPiece(Move move)
{
    switch (move.flag)
    {
    case KNIGHT_PROMO_QUIET:
    case KNIGHT_PROMO_CAPTURE:
        return KNIGHT;
    case BISHOP_PROMO_QUIET:
    case BISHOP_PROMO_CAPTURE:
        return BISHOP;
    case ROOK_PROMO_QUIET:
    case ROOK_PROMO_CAPTURE:
        return ROOK;
    case QUEEN_PROMO_QUIET:
    case QUEEN_PROMO_CAPTURE:
        return QUEEN;
    default:
        return NO_PIECE; // Not a promotion move
    }
}

static inline int isCapture(Move move)
{
    return (move.flag == CAPTURE || move.flag == EP_CAPTURE ||
            move.flag == KNIGHT_PROMO_CAPTURE || move.flag == BISHOP_PROMO_CAPTURE ||
            move.flag == ROOK_PROMO_CAPTURE || move.flag == QUEEN_PROMO_CAPTURE);
}

static inline int isPromotion(Move move)
{
    return (move.flag == KNIGHT_PROMO_QUIET || move.flag == BISHOP_PROMO_QUIET ||
            move.flag == ROOK_PROMO_QUIET || move.flag == QUEEN_PROMO_QUIET ||
            move.flag == KNIGHT_PROMO_CAPTURE || move.flag == BISHOP_PROMO_CAPTURE ||
            move.flag == ROOK_PROMO_CAPTURE || move.flag == QUEEN_PROMO_CAPTURE);
}

static inline int isEnPassant(Move move)
{
    return (move.flag == EP_CAPTURE);
}
static inline int isCastling(Move move)
{
    return (move.flag == KINGSIDE_CASTLE || move.flag == QUEENSIDE_CASTLE);
}
// ☐ Write IS_DOUBLE_PUSH(move) predicate

static inline int isDoublePush(Move move)
{
    return (move.flag == DOUBLE_PAWN_PUSH);
}

void genAllPseudoLegalMoves(CBoard *board, MoveList *moveList);
void initMoveList(MoveList *moveList);

bool isSquareAttacked(CBoard *board, Square square, Color attackerColor);
bool isKingInCheck(CBoard *board, Color side);
MoveList generateLegalMoves(CBoard *board);

#endif // MOVEGEN_H
