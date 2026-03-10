#ifndef PROPHET_MOVE_H
#define PROPHET_MOVE_H

#include <stdbool.h>
#include <stdint.h>
#include "core/chess_types.h"
#include "board/cboard.h"
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
bool move_is_capture(CBoard *board, Move move);
bool move_is_quiet(CBoard *board, Move move);
/**
 * @brief Structure to store information needed to undo a move on the chess board.
 * This includes the:
 *
 * - type of piece captured (if any)
 *
 * - square where the captured piece was (if any)
 *
 * - previous en passant square
 *
 * - previous halfmove clock for the fifty-move rule
 *
 * - previous castling rights, and the previous Zobrist hash key.
 */
typedef struct UndoInfo
{
    PieceType capturedPiece;        // What was captured (NO_PIECE if none)
    Square capturedSquare;          // Where the captured piece was (NO_SQUARE if none)
    uint8_t previousEpSquare;       // Previous en passant square (or NO_SQUARE)
    uint16_t previousHalfmoveClock; // Previous 50-move counter
    uint8_t previousCastlingRights; // 0..15 bitfield
    uint64_t previousZobristKey;
} UndoInfo;

#endif // PROPHET_MOVE_H
