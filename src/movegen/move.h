#ifndef PROPHET_MOVE_H
#define PROPHET_MOVE_H

#include "board/cboard.h"
#include "core/chess_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// NOTE: move representation adopted from stockfish

/**
 * @brief Move representation using a compact 16-bit encoding. The
 * move is represented as a uint16_t where:
 * - Bits 0-5 (6 bits): To square index (0-63 corresponding to A1-H8)
 * - Bits 6-11 (6 bits): From square index (0-63 corresponding to
 * A1-H8)
 * - Bits 12-13 (2 bits): Promotion piece type for promotion moves
 * (00=Knight, 01=Bishop, 10=Rook, 11=Queen)
 * - Bits 14-15 (2 bits): Move type flags (00=NORMAL, 01=Promotion,
 * 10=En Passant, 11=Castling) note that en passant is only present
 * when a pawn can be captured en passant
 */
typedef uint16_t Move;

// Special move value representing no move (used as a sentinel)
#define MOVE_NONE ((Move)0xFFFF)

// Move list structure
typedef struct {
    Move moves[256]; // Maximum possible moves in a position
    int count;
} MoveList;

typedef enum {
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
 * @param promo_piece The piece type for promotion moves.
 * @return Move The created move.
 */
Move create_move(Square from, Square to, MoveType type, PieceType promo_piece);
Square move_get_from_square(Move move);

Square move_get_to_square(Move move);

MoveType move_get_move_type(Move move);

PieceType move_get_promotion_piecetype(Move move);
bool move_is_enpassant(Move move);
bool move_is_promotion(Move move);
bool move_is_castling(Move move);
bool move_is_capture(CBoard* board, Move move);
bool move_is_quiet(CBoard* board, Move move);
void move_to_uci_string(Move move, char out[6]);
Move move_from_uci_string(const CBoard* board, const char* move_str, char* error_buf,
    size_t error_buf_size);
/**
 * @brief Structure to store information needed to undo a move on the
 * chess board. This includes the:
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
typedef struct UndoInfo {
    PieceType captured_piecetype; // What was captured (NO_PIECE if none)
    Square captured_square; // Where the captured piece was (NO_SQUARE
                            // if none)
    uint8_t previous_ep_square; // Previous en passant square (or
                                // NO_SQUARE)
    uint16_t previous_halfmove_clock; // Previous 50-move counter
    uint8_t previous_castling_rights; // 0..15 bitfield
    uint64_t previous_zobrist_key; // Zobrist key before the move was
                                   // made (used for undoing the move)
} UndoInfo;

#endif // PROPHET_MOVE_H
