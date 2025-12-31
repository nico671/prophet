
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"
#include "cboard.h"
#include "constant_attacks.h"
#include "sliding_attacks.h"
// Future Move representation plan:
// Bits 0-5:   FROM square (6 bits = 0-63)
// Bits 6-11:  TO square (6 bits = 0-63)
// Bits 12-15: FLAGS (4 bits = 16 possible types)
// Flag bits 12-15:

// 0000 (0)  = Quiet move
// 0001 (1)  = Double pawn push
// 0010 (2)  = Kingside castle
// 0011 (3)  = Queenside castle
// 0100 (4)  = Capture
// 0101 (5)  = EP capture

// 1000 (8)  = Knight promotion (quiet)
// 1001 (9)  = Bishop promotion (quiet)
// 1010 (10) = Rook promotion (quiet)
// 1011 (11) = Queen promotion (quiet)

// 1100 (12) = Knight promotion capture
// 1101 (13) = Bishop promotion capture
// 1110 (14) = Rook promotion capture
// 1111 (15) = Queen promotion capture
// typedef uint16_t Move;

// move flag macros
// #define IS_CAPTURE(m) (((m) >> 12) & 0x4)   // bit 2 set
// #define IS_PROMOTION(m) (((m) >> 12) & 0x8) // bit 3 set
// #define IS_CASTLE(m) ((((m) >> 12) & 0xE) == 0x2)
// #define IS_EP(m) (((m) >> 12) == 5)

// move flag
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

UndoInfo makeMove(CBoard *board, Move move);
void unmakeMove(CBoard *board, Move move, UndoInfo undoInfo);

bool isSquareAttacked(CBoard *board, Square square, Color attackerColor);

bool isKingInCheck(CBoard *board, Color side);

MoveList generateLegalMoves(CBoard *board);

#endif // MOVEGEN_H
