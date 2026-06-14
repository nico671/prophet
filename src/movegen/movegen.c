#include "movegen/movegen.h"

#include "board/cboard.h"
#include "core/bitboard.h"
#include "movegen/constant_attacks.h"
#include "movegen/constant_moves.h"
#include "movegen/move_make.h"
#include "movegen/sliding_attacks.h"
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

static bool is_square_attacked(CBoard* board, Square square, Color attacker_color)
{
    // Check for pawn attacks
    // We need to check if pawns of attacker_color can attack this square
    // If white pawns attack diagonally upward, we need to check squares diagonally downward
    // So we use the OPPOSITE color's attack pattern (FLIPPED, like in gen_all_pseudolegal_ep_pawn_moves)
    Bitboard pawnAttacks = get_pawn_attack_bitboard(square, 1 - attacker_color);
    Bitboard attackerPawns = board->piece_bbs[attacker_color][PAWN];
    if (pawnAttacks & attackerPawns)
        return true;

    // Check for knight attacks
    Bitboard knightAttacks = get_knight_attack_bitboard(square);
    Bitboard attackerKnights = board->piece_bbs[attacker_color][KNIGHT];
    if (knightAttacks & attackerKnights)
        return true;

    Bitboard attackerQueens = board->piece_bbs[attacker_color][QUEEN];
    // Check for bishop/queen attacks
    Bitboard bishopAttacks = get_bishop_attack_bitboard(square, board->occupancy_bbs[2]);
    Bitboard attackerBishops = board->piece_bbs[attacker_color][BISHOP];
    if (bishopAttacks & (attackerBishops | attackerQueens))
        return true;

    // Check for rook/queen attacks
    Bitboard rookAttacks = get_rook_attack_bitboard(square, board->occupancy_bbs[2]);
    Bitboard attackerRooks = board->piece_bbs[attacker_color][ROOK];
    if (rookAttacks & (attackerRooks | attackerQueens))
        return true;

    // Check for king attacks
    Bitboard kingAttacks = get_king_attack_bitboard(square);
    Bitboard attackerKing = board->piece_bbs[attacker_color][KING];
    if (kingAttacks & attackerKing)
        return true;

    return false;
}

bool is_king_in_check(CBoard* board, Color side)
{
    Bitboard king = board->piece_bbs[side][KING];
    if (king == 0) {
        return true;
    }
    Square king_square = bitboard_lsb_index_unsafe(king);
    Color opponent_color = 1 - side;
    return is_square_attacked(board, king_square, opponent_color);
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
            Color opponent = 1 - side;

            // Cannot castle if in check
            if (is_king_in_check(board, side)) {
                continue;
            }

            // Check squares the king moves through
            if ((side == WHITE && move_get_from_square(move) == E1 && move_get_to_square(move) == G1) || (side == BLACK && move_get_from_square(move) == E8 && move_get_to_square(move) == G8)) // KINGSIDE_CASTLE
            {
                Square through_square = (side == WHITE) ? F1 : F8;
                if (is_square_attacked(board, through_square, opponent)) {
                    continue;
                }
            } else // QUEENSIDE_CASTLE
            {
                Square through_square = (side == WHITE) ? D1 : D8;
                if (is_square_attacked(board, through_square, opponent)) {
                    continue;
                }
            }
        }

        // Normal legality check for all moves (including castling destination)
        UndoInfo undoInfo = make_move(board, move);
        if (!is_king_in_check(board, 1 - board->side_to_move)) {
            out->moves[out->count++] = move;
        }
        unmake_move(board, move, undoInfo);
    }
}