
#ifndef PROPHET_CBOARD_H
#define PROPHET_CBOARD_H

#include "chess/core/bitboard.h"
#include "chess/core/chess_types.h"

#include <stdint.h>

#pragma once

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

/**
 * @struct CBoard
 * @brief Complete chess position state used by movegen/search/eval.
 *
 * Contains piece placement, occupancy caches, game state, and the Zobrist key.
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
#define U8_CLEAR_BIT(var, pos) ((var) = (uint8_t)((var) & (uint8_t)~U8_BIT_MASK(pos)))
#define U8_CHECK_BIT(var, pos) ((uint8_t)(((var) >> (uint8_t)(pos)) & 1u))

/**
 * @brief Prints the board and its game state to standard output.
 */
void print_cboard(CBoard* board);

/**
 * @brief Parses a FEN string into a board and computes its Zobrist key.
 */
bool fen_string_to_cboard(const char* fen_string, CBoard* board);

/**
 * @brief Converts a board to a heap-allocated FEN string.
 *
 * The caller frees the returned string.
 */
char* cboard_to_fen(CBoard* board);

/**
 * @brief Returns the piece on a square, or NO_PIECE.
 */
static inline PieceType cboard_get_piece_at_square(const CBoard* board, Square square)
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
static inline void remove_piece_from_cboard(CBoard* board, Square square, Color color,
                                            PieceType piece)
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
static inline void move_piece_on_cboard(CBoard* board, Square from, Square to, Color side)
{
    PieceType moving_piece = cboard_get_piece_at_square(board, from);
    if (moving_piece == NO_PIECE) {
        return;
    }

    remove_piece_from_cboard(board, from, side, moving_piece);
    add_piece_to_cboard(board, to, side, moving_piece);
}

/**
 * @brief Removes and returns the captured piece, or NO_PIECE.
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
static inline void cboard_update_occupancies_for_capture(CBoard* board, Square square,
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
static inline void cboard_update_occupancies_for_promotion(CBoard* board, Square from,
                                                           Square to, Color color)
{
    cboard_update_occupancies_for_move(board, from, to, color);
}

/**
 * @brief Updates the occupancy bitboards when castling.
 */
static inline void cboard_update_occupancies_for_castling(CBoard* board, Square king_from,
                                                          Square king_to,
                                                          Square rook_from,
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
static inline void cboard_update_castling_rights(CBoard* board, Square from, Square to)
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
