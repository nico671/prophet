#include "attacks/constant_attacks.h"
#include "attacks/sliding_attacks.h"
#include "board/cboard.h"
#include "core/bitboard.h"
#include "movegen/constant_moves.h"
#include "movegen/move_make.h"
#include "movegen/movegen.h"
#include "movegen/sliding_moves.h"
#include <stdbool.h>

void gen_all_pseudolegal_moves(CBoard* board, MoveList* moveList) // TODO: refactor movegen to have less reused code everywhere, all individual piece gen is abt the same minus pawns and kings
{
    gen_all_pseudolegal_pawn_moves(board, moveList);
    gen_all_pseudolegal_knight_moves(board, moveList);
    gen_all_pseudolegal_bishop_moves(board, moveList);
    gen_all_pseudolegal_rook_moves(board, moveList);
    gen_all_pseudolegal_queen_moves(board, moveList);
    gen_all_pseudolegal_king_moves(board, moveList);
}

void init_move_list(MoveList* moveList)
{
    moveList->count = 0;
}

void generate_capture_moves(CBoard* board, MoveList* out)
{
    MoveList pseudoLegalMoves;
    init_move_list(&pseudoLegalMoves);
    init_move_list(out);
    gen_all_pseudolegal_moves(board, &pseudoLegalMoves);

    Color side = board->side_to_move;
    for (int i = 0; i < pseudoLegalMoves.count; i++) {
        Move move = pseudoLegalMoves.moves[i];
        bool tactical = move_is_capture(board, move) || move_is_enpassant(move) || move_is_promotion(move);
        if (!tactical) {
            continue;
        }

        UndoInfo undoInfo = make_move(board, move);
        if (!is_king_in_check(board, side)) {
            out->moves[out->count++] = move;
        }
        unmake_move(board, move, undoInfo);
    }
}

bool is_square_attacked(CBoard* board, Square square, Color attackerColor)
{
    // Check for pawn attacks
    // We need to check if pawns of attackerColor can attack this square
    // If white pawns attack diagonally upward, we need to check squares diagonally downward
    // So we use the OPPOSITE color's attack pattern (FLIPPED, like in gen_all_pseudolegal_ep_pawn_moves)
    Bitboard pawnAttacks = (attackerColor == WHITE)
        ? get_pawn_attack_bitboard(square, BLACK) // FLIPPED
        : get_pawn_attack_bitboard(square, WHITE); // FLIPPED
    Bitboard attackerPawns = (attackerColor == WHITE) ? board->white_pawns_bb : board->black_pawns_bb;
    if (pawnAttacks & attackerPawns)
        return true;

    // Check for knight attacks
    Bitboard knightAttacks = get_knight_attack_bitboard(square);
    Bitboard attackerKnights = (attackerColor == WHITE) ? board->white_knights_bb : board->black_knights_bb;
    if (knightAttacks & attackerKnights)
        return true;

    // Check for bishop/queen attacks
    Bitboard bishopAttacks = get_bishop_attack_bitboard(square, board->all_pieces_bb);
    Bitboard attackerBishops = (attackerColor == WHITE) ? board->white_bishops_bb : board->black_bishops_bb;
    Bitboard attackerQueens = (attackerColor == WHITE) ? board->white_queens_bb : board->black_queens_bb;
    if (bishopAttacks & (attackerBishops | attackerQueens))
        return true;

    // Check for rook/queen attacks
    Bitboard rookAttacks = get_rook_attack_bitboard(square, board->all_pieces_bb);
    Bitboard attackerRooks = (attackerColor == WHITE) ? board->white_rooks_bb : board->black_rooks_bb;
    if (rookAttacks & (attackerRooks | attackerQueens))
        return true;

    // Check for king attacks
    Bitboard kingAttacks = get_king_attack_bitboard(square);
    Bitboard attackerKing = (attackerColor == WHITE) ? board->white_king_bb : board->black_king_bb;
    if (kingAttacks & attackerKing)
        return true;

    return false;
}

bool is_king_in_check(CBoard* board, Color side)
{
    Bitboard king = (side == WHITE) ? board->white_king_bb : board->black_king_bb;
    if (king == 0) {
        return true;
    }
    Square kingSquare = bitboard_lsb_index(king);
    Color opponentColor = (side == WHITE) ? BLACK : WHITE;
    return is_square_attacked(board, kingSquare, opponentColor);
}

void generate_legal_moves(CBoard* board, MoveList* out)
{
    MoveList pseudoLegalMoves;
    init_move_list(&pseudoLegalMoves);
    gen_all_pseudolegal_moves(board, &pseudoLegalMoves);

    for (int i = 0; i < pseudoLegalMoves.count; i++) {
        Move move = pseudoLegalMoves.moves[i];

        // Special handling for castling
        if (move_is_castling(move)) {
            Color side = board->side_to_move;
            Color opponent = (side == WHITE) ? BLACK : WHITE;
            // Square kingFrom = FROM_SQ(move);

            // Cannot castle if in check
            if (is_king_in_check(board, side)) {
                continue;
            }

            // Check squares the king moves through
            if ((side == WHITE && move_get_from_square(move) == E1 && move_get_to_square(move) == G1) || (side == BLACK && move_get_from_square(move) == E8 && move_get_to_square(move) == G8)) // KINGSIDE_CASTLE
            {
                Square throughSquare = (side == WHITE) ? F1 : F8;
                if (is_square_attacked(board, throughSquare, opponent)) {
                    continue;
                }
            } else // QUEENSIDE_CASTLE
            {
                Square throughSquare = (side == WHITE) ? D1 : D8;
                if (is_square_attacked(board, throughSquare, opponent)) {
                    continue;
                }
            }
        }

        // Normal legality check for all moves (including castling destination)
        UndoInfo undoInfo = make_move(board, move);
        if (!is_king_in_check(board, (board->side_to_move == WHITE) ? BLACK : WHITE)) {
            out->moves[out->count++] = move;
        }
        unmake_move(board, move, undoInfo);
    }
}