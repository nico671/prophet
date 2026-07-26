
#ifndef PROPHET_CBOARD_H
#define PROPHET_CBOARD_H

#include "core/bitboard.h"
#include "core/chess_types.h"

#include <stdint.h>

#pragma once

#define START_FEN \
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

/**
 * @struct CBoard
 * @brief Complete chess position state used by movegen/search/eval.
 *
 * Stores per-piece bitboards, cached occupancies, side-to-move and
 * rule metadata, plus an incremental Zobrist hash.
 *
 * Field groups:
 *
 * - Piece placements: piece_bbs[2][7] indexed by color and piece type
 * for compact storage
 *
 * - Occupancies: occupancy_bbs[3] indexed by color then 2 = all, for
 * quick access to occupancy bitboards without needing to OR piece
 * bitboards together
 *
 * - Square piece mapping: piece_at_square[64] for O(1) piece type
 * retrieval at any square without bitboard checks
 *
 * - State metadata: side_to_move, castling_rights, ep_square,
 * half_move_clock, full_move_number
 *
 * - Hash: zobrist_key
 */
typedef struct CBoard {
    // --- Piece Placement ---
    Bitboard piece_bbs[2][7]; /**< 2D array of bitboards indexed by color
                                 and piece type for compact storage. Uses
                                 [2][7] because NO_PIECE = 0, and would
                                 rather waste 16 bytes of memory than have
                                 to do - 1 operation required for [2][6] */

    // --- Occupancy Cache ---
    Bitboard occupancy_bbs[3]; /**< Array of occupancy bitboards
                                  indexed by color then 2 = all */

    // --- Square Piece Mapping ---
    PieceType piece_at_square[64]; /**< Array mapping each square index to
                                      the piece type occupying it, or NONE
                                      if empty */

    // --- Game State Metadata ---
    Color side_to_move; /** 0 for White, 1 for Black */

    /**
     * @brief Castling rights bitfield.
     * Bits: 0:BQ, 1:BK, 2:WQ, 3:WK
     */
    uint8_t castling_rights;

    uint8_t ep_square; /**< Index (0-63) or 64 if none available */
    uint16_t half_move_clock; /**< Counter for the fifty-move rule */
    uint16_t full_move_number; /**< Incremented after every Black move */

    // --- Optimization ---
    uint64_t zobrist_key; /**< Unique hash key for the current position */
} CBoard;

// Bit manipulation macros for castling rights, stored in a single
// uint8_t for compactness
#define U8_BIT_MASK(pos) ((uint8_t)(1u << (uint8_t)(pos)))
#define U8_SET_BIT(var, pos) ((var) = (uint8_t)((var) | U8_BIT_MASK(pos)))
#define U8_CLEAR_BIT(var, pos) \
    ((var) = (uint8_t)((var) & (uint8_t)~U8_BIT_MASK(pos)))
#define U8_CHECK_BIT(var, pos) ((uint8_t)(((var) >> (uint8_t)(pos)) & 1u))

/**
 * @brief Prints a human-readable representation of a chess board to
 * standard output, displaying the piece layout from rank 8 to rank 1
 * using standard chess notation characters (uppercase for white,
 * lowercase for black, '.' for empty squares), followed by game state
 * information including side to move, en passant square, halfmove
 * clock, fullmove number, castling rights, and the Zobrist hash key.
 *
 * Returns early with an error message if the provided CBoard pointer
 * is NULL.
 *
 * @param board Pointer to the CBoard struct to print.
 */
void print_cboard(CBoard* board);

/**
 * @brief Parses a FEN string and initializes a CBoard struct with the
 * corresponding piece placements, game state information (side to
 * move, castling rights, en passant square, halfmove clock, fullmove
 * number), and computes the Zobrist hash key for the resulting board
 * state. The function validates the FEN string format and returns
 * false if the string is invalid (e.g., incorrect piece placement,
 * missing sections, invalid characters). On success, it populates the
 * provided CBoard struct with the parsed information and returns
 * true.
 *
 * @param fen_string A null-terminated string in Forsyth-Edwards
 * Notation representing a chess position, including piece placements,
 * side to move, castling rights, en passant square, halfmove clock,
 * and fullmove number.
 * @param board Pointer to a CBoard struct that will be initialized
 * based on the provided FEN string. The function will populate all
 * fields of the CBoard struct according to the information encoded in
 * the FEN string.
 * @return bool True if the FEN string was successfully parsed and the
 * CBoard struct was initialized, false otherwise.
 */
bool fen_string_to_cboard(const char* fen_string, CBoard* board);

/**
 * @brief Converts a CBoard struct back into a FEN string
 * representation, including piece placements, side to move, castling
 * rights, en passant square, halfmove clock, and fullmove number. The
 * returned string is dynamically allocated and should be freed by the
 * caller.
 *
 * @param board Pointer to the CBoard struct to convert to FEN.
 * @return char* Dynamically allocated null-terminated string in FEN
 * format representing the given CBoard position. Caller is
 * responsible for freeing this memory.
 */
char* cboard_to_fen(CBoard* board);

/**
 * @brief Returns the piece type occupying the given square on the
 * board, or NO_PIECE if the square is empty. The function uses the
 * piece_at_square array in the CBoard struct, which is indexed by the
 * square (0-63 corresponding to A1-H8) and contains the piece type
 * for each square. This allows for O(1) retrieval of the piece type
 * at any given square without needing to check individual bitboards.
 *
 * @param board Pointer to the CBoard struct representing the current
 * board state.
 * @param square The square for which to retrieve the piece type.
 * @return PieceType The piece type occupying the given square, or
 * NO_PIECE if the square is empty.
 */
static inline PieceType cboard_get_piece_at_square(const CBoard* board,
                                                   Square square)
{
    if (!board || square < A1 || square >= NO_SQUARE) {
        return NO_PIECE;
    }

    // Mailbox-backed lookup with occupancy validation.
    if (!bitboard_is_bit_set(board->occupancy_bbs[2], square)) {
        return NO_PIECE;
    }

    return board->piece_at_square[square];
}

/**
 * @brief Adds a piece to the board at the specified square.
 *
 * @param board Pointer to the CBoard struct.
 * @param square The square where the piece will be added.
 * @param color The color of the piece to add.
 * @param piece The type of piece to add.
 */
static inline void add_piece_to_cboard(CBoard* board, Square square, Color color,
                                       PieceType piece)
{
    Bitboard* bb = &board->piece_bbs[color][piece];
    if (!bb || square >= NO_SQUARE) {
        return;
    }

    bitboard_set_square_bit(bb, square);
    board->piece_at_square[square] = piece;
}

/**
 * @brief Removes a piece from the board at the specified square.
 *
 * @param board Pointer to the CBoard struct.
 * @param square The square from which the piece will be removed.
 * @param color The color of the piece to remove.
 * @param piece The type of piece to remove.
 */
static inline void remove_piece_from_cboard(CBoard* board, Square square,
                                            Color color, PieceType piece)
{
    Bitboard* bb = &board->piece_bbs[color][piece];
    if (!bb || square >= NO_SQUARE) {
        return;
    }

    bitboard_clear_square_bit(bb, square);
    board->piece_at_square[square] = NO_PIECE;
}

/**
 * @brief Moves a piece from one square to another on the board.
 *
 * @param board Pointer to the CBoard struct.
 * @param from The square from which the piece will be moved.
 * @param to The square to which the piece will be moved.
 * @param side The color of the piece being moved.

*/
static inline void move_piece_on_cboard(CBoard* board, Square from, Square to,
                                        Color side)
{
    PieceType moving_piece = cboard_get_piece_at_square(board, from);
    if (moving_piece == NO_PIECE) {
        return;
    }

    remove_piece_from_cboard(board, from, side, moving_piece);
    add_piece_to_cboard(board, to, side, moving_piece);
}

/**
 * @brief Removes a captured piece from the board and returns its
 type.
 *
 * @param board Pointer to the CBoard struct.
 * @param square The square from which the piece will be removed.
 * @param capturing_color The color of the piece that captured the
 target piece.
 * @return PieceType The type of the captured piece, or NO_PIECE if no
 piece was captured.

 */
static inline PieceType cboard_remove_captured_piece(CBoard* board, Square square,
                                                     Color capturing_color)
{
    Color captured_color = 1 - capturing_color;

    PieceType captured_piece = cboard_get_piece_at_square(board, square);
    if (captured_piece == NO_PIECE) {
        return NO_PIECE;
    }

    remove_piece_from_cboard(board, square, captured_color, captured_piece);
    return captured_piece;
}
/**
 * @brief Updates the occupancy bitboards when moving a piece from one
 * square to another.
 *
 * @param board Pointer to the CBoard struct.
 * @param from The square from which the piece will be moved.
 * @param to The square to which the piece will be moved.
 * @param color The color of the piece being moved.
 */
static inline void cboard_update_occupancies_for_move(CBoard* board, Square from,
                                                      Square to, Color color)
{
    bitboard_clear_square_bit(&board->occupancy_bbs[color], from);
    bitboard_set_square_bit(&board->occupancy_bbs[color], to);
    bitboard_clear_square_bit(&board->occupancy_bbs[2], from);
    bitboard_set_square_bit(&board->occupancy_bbs[2], to);
}

/**
 * @brief Updates the occupancy bitboards when capturing a piece.
 *
 * @param board Pointer to the CBoard struct.
 * @param square The square from which the piece will be captured.
 * @param capturedColor The color of the piece being captured.
 */
static inline void cboard_update_occupancies_for_capture(CBoard* board,
                                                         Square square,
                                                         Color captured_color)
{
    bitboard_clear_square_bit(&board->occupancy_bbs[captured_color], square);
    bitboard_clear_square_bit(&board->occupancy_bbs[2], square);
}

/**
 * @brief Updates the occupancy bitboards when promoting a pawn. Same
 * as a regular move - color occupancy changes from 'from' to 'to'
 *
 * @param board Pointer to the CBoard struct.
 * @param from The square from which the pawn will be moved.
 * @param to The square to which the pawn will be moved.
 * @param color The color of the piece being moved.
 */
static inline void cboard_update_occupancies_for_promotion(CBoard* board,
                                                           Square from, Square to,
                                                           Color color)
{
    cboard_update_occupancies_for_move(board, from, to, color);
}

/**
 * @brief Updates the occupancy bitboards when castling.
 *
 * @param board Pointer to the CBoard struct.
 * @param king_from The square from which the king will be moved.
 * @param king_to The square to which the king will be moved.
 * @param rook_from The square from which the rook will be moved.
 * @param rook_to The square to which the rook will be moved.
 * @param color The color of the pieces being moved.
 */
static inline void
cboard_update_occupancies_for_castling(CBoard* board, Square king_from,
                                       Square king_to, Square rook_from,
                                       Square rook_to, Color color)
{
    bitboard_clear_square_bit(&board->occupancy_bbs[color], king_from);
    bitboard_clear_square_bit(&board->occupancy_bbs[color], rook_from);
    bitboard_set_square_bit(&board->occupancy_bbs[color], king_to);
    bitboard_set_square_bit(&board->occupancy_bbs[color], rook_to);
    bitboard_clear_square_bit(&board->occupancy_bbs[2], king_from);
    bitboard_clear_square_bit(&board->occupancy_bbs[2], rook_from);
    bitboard_set_square_bit(&board->occupancy_bbs[2], king_to);
    bitboard_set_square_bit(&board->occupancy_bbs[2], rook_to);
}

/**
 * @brief Updates the castling rights when a rook or king moves.
 *
 * @param board Pointer to the CBoard struct.
 * @param from The square from which the piece will be moved.
 * @param to The square to which the piece will be moved.
 */
static inline void cboard_update_castling_rights(CBoard* board, Square from,
                                                 Square to)
{
    // If king moved, lose all castling
    if (bitboard_is_bit_set(board->piece_bbs[WHITE][KING], to)) {
        U8_CLEAR_BIT(board->castling_rights, 3);
        U8_CLEAR_BIT(board->castling_rights, 2);
    } else if (bitboard_is_bit_set(board->piece_bbs[BLACK][KING], to)) {
        U8_CLEAR_BIT(board->castling_rights, 1);
        U8_CLEAR_BIT(board->castling_rights, 0);
    }

    // If rook moved from corner, lose that side's castling
    if (from == H1)
        U8_CLEAR_BIT(board->castling_rights, 3);
    if (from == A1)
        U8_CLEAR_BIT(board->castling_rights, 2);
    if (from == H8)
        U8_CLEAR_BIT(board->castling_rights, 1);
    if (from == A8)
        U8_CLEAR_BIT(board->castling_rights, 0);

    // If rook was captured on corner square, lose that side's
    // castling
    if (to == H1)
        U8_CLEAR_BIT(board->castling_rights, 3);
    if (to == A1)
        U8_CLEAR_BIT(board->castling_rights, 2);
    if (to == H8)
        U8_CLEAR_BIT(board->castling_rights, 1);
    if (to == A8)
        U8_CLEAR_BIT(board->castling_rights, 0);
}
#endif // CBOARD_H
