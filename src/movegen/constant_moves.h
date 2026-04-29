#ifndef CONSTANT_MOVES_H
#define CONSTANT_MOVES_H

#include "movegen/move.h"

typedef struct CBoard CBoard;

/**
 * @brief Generates all pseudo-legal non-castling king moves for the given board.
 *
 * @param board The current game board.
 * @param moveList The list to which generated moves will be added.
 */
void genAllPseudoLegalKingNonCastlingMoves(CBoard* board, MoveList* moveList);

/**
 * @brief Generates all pseudo-legal castling king moves for the given board.
 *
 * @param board The current game board.
 * @param moveList The list to which generated moves will be added.
 */
void genAllPseudoLegalKingCastlingMoves(CBoard* board, MoveList* moveList);
/**
 * @brief Generates all pseudo-legal king moves for the given board.
 *
 * @param board The current game board.
 * @param moveList The list to which generated moves will be added.
 */
void genAllPseudoLegalKingMoves(CBoard* board, MoveList* moveList);

/**
 * @brief Generates all pseudo-legal knight moves for the given board.
 *
 * @param board The current game board.
 * @param moveList The list to which generated moves will be added.
 */
void genAllPseudoLegalKnightMoves(CBoard* board, MoveList* moveList);

/**
 * @brief Generates all single pawn pushes for the given board.
 *
 * @param board The current game board.
 * @param moveList The list to which generated moves will be added.
 */
void genSinglePawnPushes(CBoard* board, MoveList* moveList);
/**
 * @brief Generates all double pawn pushes for the given board.
 *
 * @param board The current game board.
 * @param moveList The list to which generated moves will be added.
 */
void genDoublePawnPushes(CBoard* board, MoveList* moveList);
/**
 * @brief Generates all pawn captures for the given board.
 *
 * @param board The current game board.
 * @param moveList The list to which generated moves will be added.
 */
void genPawnCaptures(CBoard* board, MoveList* moveList);
/**
 * @brief Generates all pawn promotions for the given board.
 *
 * @param board The current game board.
 * @param moveList The list to which generated moves will be added.
 */
void genPawnPromotions(CBoard* board, MoveList* moveList);
/**
 * @brief Generates all en passant pawn moves for the given board.
 *
 * @param board The current game board.
 * @param moveList The list to which generated moves will be added.
 */
void genEnPassantPawnMoves(CBoard* board, MoveList* moveList);
/**
 * @brief Generates all pseudo-legal pawn moves for the given board.
 *
 * @param board The current game board.
 * @param moveList The list to which generated moves will be added.
 */
void genAllPseudoLegalPawnMoves(CBoard* board, MoveList* moveList);
#endif // CONSTANT_MOVES_H