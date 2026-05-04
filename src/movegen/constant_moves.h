#ifndef CONSTANT_MOVES_H
#define CONSTANT_MOVES_H

#include "movegen/move.h"

typedef struct CBoard CBoard;

/**
 * @brief Generates all pseudo-legal non-castling king moves for the given board.
 *
 * @param board The current game board.
 * @param move_list The list to which generated moves will be added.
 */
void gen_all_pseudolegal_king_noncastling_moves(CBoard* board, MoveList* move_list);

/**
 * @brief Generates all pseudo-legal castling king moves for the given board.
 *
 * @param board The current game board.
 * @param move_list The list to which generated moves will be added.
 */
void gen_all_pseudolegal_king_castling_moves(CBoard* board, MoveList* move_list);
/**
 * @brief Generates all pseudo-legal king moves for the given board.
 *
 * @param board The current game board.
 * @param move_list The list to which generated moves will be added.
 */
void gen_all_pseudolegal_king_moves(CBoard* board, MoveList* move_list);

/**
 * @brief Generates all pseudo-legal knight moves for the given board.
 *
 * @param board The current game board.
 * @param move_list The list to which generated moves will be added.
 */
void gen_all_pseudolegal_knight_moves(CBoard* board, MoveList* move_list);

/**
 * @brief Generates all single pawn pushes for the given board.
 *
 * @param board The current game board.
 * @param move_list The list to which generated moves will be added.
 */
void gen_all_pseudolegal_single_pawn_pushes(CBoard* board, MoveList* move_list);
/**
 * @brief Generates all double pawn pushes for the given board.
 *
 * @param board The current game board.
 * @param move_list The list to which generated moves will be added.
 */
void gen_all_pseudolegal_double_pawn_pushes(CBoard* board, MoveList* move_list);
/**
 * @brief Generates all pawn captures for the given board.
 *
 * @param board The current game board.
 * @param move_list The list to which generated moves will be added.
 */
void gen_all_pseudolegal_pawn_captures(CBoard* board, MoveList* move_list);
/**
 * @brief Generates all pawn promotions for the given board.
 *
 * @param board The current game board.
 * @param move_list The list to which generated moves will be added.
 */
void gen_all_pseudolegal_pawn_promotions(CBoard* board, MoveList* move_list);
/**
 * @brief Generates all en passant pawn moves for the given board.
 *
 * @param board The current game board.
 * @param move_list The list to which generated moves will be added.
 */
void gen_all_pseudolegal_ep_pawn_moves(CBoard* board, MoveList* move_list);
/**
 * @brief Generates all pseudo-legal pawn moves for the given board.
 *
 * @param board The current game board.
 * @param move_list The list to which generated moves will be added.
 */
void gen_all_pseudolegal_pawn_moves(CBoard* board, MoveList* move_list);
#endif // CONSTANT_MOVES_H