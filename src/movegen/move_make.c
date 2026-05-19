#include "board/cboard.h"
#include "board/zobrist.h"
#include "core/bitboard.h"
#include "movegen/move.h"
#include "movegen/move_make.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Helper to save undo info
static UndoInfo save_undo_info(CBoard* board, PieceType captured_piecetype, Move move)
{
    UndoInfo undo_info = { 0 };
    undo_info.captured_piecetype = captured_piecetype;
    undo_info.captured_square = (captured_piecetype != NO_PIECE) ? move_get_to_square(move) : NO_SQUARE;
    undo_info.previous_ep_square = board->ep_square;
    undo_info.previous_halfmove_clock = board->half_move_clock;
    undo_info.previous_castling_rights = board->castling_rights;
    return undo_info;
}

// Helper to update game state after move
static void updateGameState(CBoard* board, Square to, bool is_capture)
{
    // Update halfmove clock
    bool is_pawn_move = bitboard_is_bit_set(board->piece_bbs[WHITE][PAWN], to) || bitboard_is_bit_set(board->piece_bbs[BLACK][PAWN], to);
    if (is_pawn_move || is_capture) {
        board->half_move_clock = 0;
    } else {
        board->half_move_clock++;
    }

    // Clear en passant square
    board->ep_square = NO_SQUARE;

    // Switch sides
    board->side_to_move = 1 - board->side_to_move;

    // Increment fullmove after black moves
    if (board->side_to_move == WHITE) {
        board->full_move_number++;
    }
}

UndoInfo make_quiet_move(CBoard* board, Move move)
{
    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);

    // Save undo info before making changes
    UndoInfo undo_info = save_undo_info(board, NO_PIECE, move);
    undo_info.previous_zobrist_key = board->zobrist_key;

    // Determine the piece type being moved
    PieceType moving_piecetype = NO_PIECE;
    moving_piecetype = cboard_get_piece_at_square(board, from);

    // Update zobrist: remove piece from 'from' square
    zobrist_toggle_piece(&board->zobrist_key, moving_piecetype, board->side_to_move, from);

    // Remove old EP square from hash if any
    zobrist_toggle_ep(&board->zobrist_key, board, board->ep_square);

    // Save old castling rights
    uint8_t previous_castling_rights = board->castling_rights;

    // Move the piece
    move_piece_on_cboard(board, from, to, board->side_to_move);

    // Update board state
    cboard_update_castling_rights(board, from, to);
    cboard_update_occupancies_for_move(board, from, to, board->side_to_move);

    // Update zobrist: add piece to 'to' square
    zobrist_toggle_piece(&board->zobrist_key, moving_piecetype, board->side_to_move, to);

    // Update zobrist for castling rights change
    zobrist_toggle_castling(&board->zobrist_key, previous_castling_rights, board->castling_rights);

    updateGameState(board, to, false);

    // Toggle side to move in zobrist (done after updateGameState which switches side)
    zobrist_toggle_side(&board->zobrist_key);

    return undo_info;
}

UndoInfo make_capture_move(CBoard* board, Move move)
{
    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);

    // Determine the piece type being moved
    PieceType moving_piecetype = NO_PIECE;
    moving_piecetype = cboard_get_piece_at_square(board, from);

    // Remove captured_piecetype piece first
    PieceType captured_piecetype = cboard_remove_captured_piece(board, to, board->side_to_move);

    // Save undo info
    UndoInfo undo_info = save_undo_info(board, captured_piecetype, move);
    undo_info.previous_zobrist_key = board->zobrist_key;

    // Update zobrist: remove moving piece from 'from' square
    zobrist_toggle_piece(&board->zobrist_key, moving_piecetype, board->side_to_move, from);

    // Update zobrist: remove captured_piecetype piece from 'to' square
    Color captured_color = 1 - board->side_to_move;
    zobrist_toggle_piece(&board->zobrist_key, captured_piecetype, captured_color, to);

    // Remove old EP square from hash if any
    zobrist_toggle_ep(&board->zobrist_key, board, board->ep_square);

    // Save old castling rights
    uint8_t previous_castling_rights = board->castling_rights;

    // Update occupancies for capture (remove captured_piecetype piece)
    cboard_update_occupancies_for_capture(board, to, captured_color);

    // Move the capturing piece
    move_piece_on_cboard(board, from, to, board->side_to_move);

    // Update board state
    cboard_update_castling_rights(board, from, to);
    cboard_update_occupancies_for_move(board, from, to, board->side_to_move);

    // Update zobrist: add moving piece to 'to' square
    zobrist_toggle_piece(&board->zobrist_key, moving_piecetype, board->side_to_move, to);

    // Update zobrist for castling rights change
    zobrist_toggle_castling(&board->zobrist_key, previous_castling_rights, board->castling_rights);

    updateGameState(board, to, true);

    // Toggle side to move in zobrist
    zobrist_toggle_side(&board->zobrist_key);

    return undo_info;
}

UndoInfo make_double_pawn_push_move(CBoard* board, Move move)
{
    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);

    // Save undo info before making any changes
    UndoInfo undo_info = save_undo_info(board, NO_PIECE, move);
    undo_info.previous_zobrist_key = board->zobrist_key;

    // Calculate en passant square BEFORE switching sides
    // EP square is the square the pawn skipped over
    Square ep_square = to + (8 * (2 * board->side_to_move - 1)); // to + 8 for white, to - 8 for black

    // Update zobrist: remove pawn from 'from' square
    zobrist_toggle_piece(&board->zobrist_key, PAWN, board->side_to_move, from);

    // Remove old EP square from hash if any
    zobrist_toggle_ep(&board->zobrist_key, board, board->ep_square);

    // Save old castling rights
    uint8_t previous_castling_rights = board->castling_rights;

    // Move the pawn
    move_piece_on_cboard(board, from, to, board->side_to_move);

    // Update board state
    cboard_update_castling_rights(board, from, to);
    cboard_update_occupancies_for_move(board, from, to, board->side_to_move);

    // Update zobrist: add pawn to 'to' square
    zobrist_toggle_piece(&board->zobrist_key, PAWN, board->side_to_move, to);

    // Update zobrist for castling rights change
    zobrist_toggle_castling(&board->zobrist_key, previous_castling_rights, board->castling_rights);

    updateGameState(board, to, false); // This switches sideToMove and clears ep_square

    // Set en passant square AFTER updateGameState (which clears it)
    board->ep_square = ep_square;

    // Add new EP square to hash
    zobrist_toggle_ep(&board->zobrist_key, board, ep_square);

    // Toggle side to move in zobrist
    zobrist_toggle_side(&board->zobrist_key);

    return undo_info;
}

UndoInfo make_ep_capture_move(CBoard* board, Move move)
{
    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);

    // Determine the square of the captured_piecetype pawn
    Square captured_pawn_square = to + (8 * (2 * board->side_to_move - 1));

    UndoInfo undo_info = save_undo_info(board, PAWN, move);
    undo_info.previous_zobrist_key = board->zobrist_key;

    // Update zobrist: remove capturing pawn from 'from' square
    zobrist_toggle_piece(&board->zobrist_key, PAWN, board->side_to_move, from);

    // Update zobrist: remove captured_piecetype pawn from its square
    Color captured_color = 1 - board->side_to_move;
    zobrist_toggle_piece(&board->zobrist_key, PAWN, captured_color, captured_pawn_square);

    // Remove old EP square from hash
    zobrist_toggle_ep(&board->zobrist_key, board, board->ep_square);

    // Save old castling rights
    uint8_t previous_castling_rights = board->castling_rights;

    // Remove the captured_piecetype pawn
    cboard_remove_captured_piece(board, captured_pawn_square, board->side_to_move);

    // Update occupancies for capture (remove captured_piecetype pawn)
    cboard_update_occupancies_for_capture(board, captured_pawn_square, captured_color);

    // Move the capturing pawn
    move_piece_on_cboard(board, from, to, board->side_to_move);

    // Update board state
    cboard_update_castling_rights(board, from, to);
    cboard_update_occupancies_for_move(board, from, to, board->side_to_move);

    // Update zobrist: add capturing pawn to 'to' square
    zobrist_toggle_piece(&board->zobrist_key, PAWN, board->side_to_move, to);

    // Update zobrist for castling rights change
    zobrist_toggle_castling(&board->zobrist_key, previous_castling_rights, board->castling_rights);

    updateGameState(board, to, true);

    // Toggle side to move in zobrist
    zobrist_toggle_side(&board->zobrist_key);

    return undo_info;
}

UndoInfo make_promotion_move(CBoard* board, Move move)
{
    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);

    // Determine if it's a promotion capture
    bool is_promotion_capture;
    if (board->side_to_move == WHITE) {
        is_promotion_capture = bitboard_is_bit_set(board->occupancy_bbs[BLACK], to);
    } else {
        is_promotion_capture = bitboard_is_bit_set(board->occupancy_bbs[WHITE], to);
    }

    // If capture, remove the captured_piecetype piece first and save its type
    PieceType captured_piecetype = NO_PIECE;
    if (is_promotion_capture) {
        captured_piecetype = cboard_remove_captured_piece(board, to, board->side_to_move);
        // Update occupancies for capture
        Color captured_color = 1 - board->side_to_move;
        cboard_update_occupancies_for_capture(board, to, captured_color);
    }

    // Save undo info before making changes
    UndoInfo undo_info = save_undo_info(board, captured_piecetype, move);
    undo_info.previous_zobrist_key = board->zobrist_key;

    // Get the promotion piece type
    PieceType promotion_piecetype = move_get_promotion_piecetype(move);

    // Update zobrist: remove pawn from 'from' square
    zobrist_toggle_piece(&board->zobrist_key, PAWN, board->side_to_move, from);

    // Update zobrist: if capture, remove captured_piecetype piece from 'to' square
    if (is_promotion_capture && captured_piecetype != NO_PIECE) {
        Color captured_color = 1 - board->side_to_move;
        zobrist_toggle_piece(&board->zobrist_key, captured_piecetype, captured_color, to);
    }

    // Remove old EP square from hash
    zobrist_toggle_ep(&board->zobrist_key, board, board->ep_square);

    // Save old castling rights
    uint8_t previous_castling_rights = board->castling_rights;

    // Replace pawn with promoted piece
    remove_piece_from_cboard(board, from, board->side_to_move, PAWN);
    add_piece_to_cboard(board, to, board->side_to_move, promotion_piecetype);

    // Update zobrist: add promoted piece to 'to' square
    zobrist_toggle_piece(&board->zobrist_key, promotion_piecetype, board->side_to_move, to);

    // Update board state
    cboard_update_castling_rights(board, from, to);
    cboard_update_occupancies_for_promotion(board, from, to, board->side_to_move);

    // Update zobrist for castling rights change
    zobrist_toggle_castling(&board->zobrist_key, previous_castling_rights, board->castling_rights);

    updateGameState(board, to, is_promotion_capture);

    // Toggle side to move in zobrist
    zobrist_toggle_side(&board->zobrist_key);

    return undo_info;
}

UndoInfo make_castling_move(CBoard* board, Move move)
{
    Square to = move_get_to_square(move);
    Square from = move_get_from_square(move);
    // Save undo info before making changes
    UndoInfo undo_info = save_undo_info(board, NO_PIECE, move);
    undo_info.previous_zobrist_key = board->zobrist_key;

    // Remove old EP square from hash
    zobrist_toggle_ep(&board->zobrist_key, board, board->ep_square);

    // Save old castling rights
    uint8_t previous_castling_rights = board->castling_rights;

    if (board->side_to_move == WHITE) {
        // check if kingside castle and handle accordingly (king moves e1->g1, rook moves h1->f1)
        if (from == E1 && to == G1) {
            // Update zobrist: remove pieces from original squares
            zobrist_toggle_piece(&board->zobrist_key, KING, WHITE, E1);
            zobrist_toggle_piece(&board->zobrist_key, ROOK, WHITE, H1);

            // Move king from e1 to g1 and rook from h1 to f1
            move_piece_on_cboard(board, E1, G1, WHITE);
            move_piece_on_cboard(board, H1, F1, WHITE);
            // Update occupancies incrementally
            cboard_update_occupancies_for_castling(board, E1, G1, H1, F1, WHITE);

            // Update zobrist: add pieces to new squares
            zobrist_toggle_piece(&board->zobrist_key, KING, WHITE, G1);
            zobrist_toggle_piece(&board->zobrist_key, ROOK, WHITE, F1);
        } else { // handle queenside castle (king moves e1->c1, rook moves a1->d1)
            // Update zobrist: remove pieces from original squares
            zobrist_toggle_piece(&board->zobrist_key, KING, WHITE, E1);
            zobrist_toggle_piece(&board->zobrist_key, ROOK, WHITE, A1);

            // Move king from e1 to c1 and rook from a1 to d1
            move_piece_on_cboard(board, E1, C1, WHITE);
            move_piece_on_cboard(board, A1, D1, WHITE);
            // Update occupancies incrementally
            cboard_update_occupancies_for_castling(board, E1, C1, A1, D1, WHITE);

            // Update zobrist: add pieces to new squares
            zobrist_toggle_piece(&board->zobrist_key, KING, WHITE, C1);
            zobrist_toggle_piece(&board->zobrist_key, ROOK, WHITE, D1);
        }
    } else {
        if (from == E8 && to == G8) {
            // Update zobrist: remove pieces from original squares
            zobrist_toggle_piece(&board->zobrist_key, KING, BLACK, E8);
            zobrist_toggle_piece(&board->zobrist_key, ROOK, BLACK, H8);

            // Move king from e8 to g8 and rook from h8 to f8
            move_piece_on_cboard(board, E8, G8, BLACK);
            move_piece_on_cboard(board, H8, F8, BLACK);
            // Update occupancies incrementally
            cboard_update_occupancies_for_castling(board, E8, G8, H8, F8, BLACK);

            // Update zobrist: add pieces to new squares
            zobrist_toggle_piece(&board->zobrist_key, KING, BLACK, G8);
            zobrist_toggle_piece(&board->zobrist_key, ROOK, BLACK, F8);
        } else { // handle queenside castle (king moves e8->c8, rook moves a8->d8)
            // Update zobrist: remove pieces from original squares
            zobrist_toggle_piece(&board->zobrist_key, KING, BLACK, E8);
            zobrist_toggle_piece(&board->zobrist_key, ROOK, BLACK, A8);
            // Move king from e8 to c8 and rook from a8 to d8
            move_piece_on_cboard(board, E8, C8, BLACK);
            move_piece_on_cboard(board, A8, D8, BLACK);
            // Update occupancies incrementally
            cboard_update_occupancies_for_castling(board, E8, C8, A8, D8, BLACK);
            // Update zobrist: add pieces to new squares
            zobrist_toggle_piece(&board->zobrist_key, KING, BLACK, C8);
            zobrist_toggle_piece(&board->zobrist_key, ROOK, BLACK, D8);
        }
    }
    // Update castling rights (castling removes all rights for this color)
    if (board->side_to_move == WHITE) {
        U8_CLEAR_BIT(board->castling_rights, 3);
        U8_CLEAR_BIT(board->castling_rights, 2);
    } else {
        U8_CLEAR_BIT(board->castling_rights, 1);
        U8_CLEAR_BIT(board->castling_rights, 0);
    }

    // Update zobrist for castling rights change
    zobrist_toggle_castling(&board->zobrist_key, previous_castling_rights, board->castling_rights);

    // Update board state (occupancies already updated above)
    updateGameState(board, to, false);

    // Toggle side to move in zobrist
    zobrist_toggle_side(&board->zobrist_key);

    return undo_info;
}

UndoInfo make_move(CBoard* board, Move move)
{
    // extract flag
    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);
    PieceType moving_piecetype = cboard_get_piece_at_square(board, from);

    if (move_is_castling(move)) {
        if (board->side_to_move == WHITE) {
            if (to == G1) {
                return make_castling_move(board, move); // Kingside castle
            } else {
                return make_castling_move(board, move); // Queenside castle
            }
        } else {
            if (to == G8) {
                return make_castling_move(board, move); // Kingside castle
            } else {
                return make_castling_move(board, move); // Queenside castle
            }
        }
    } else if (move_is_enpassant(move)) {
        // since ep flag is set if en passant is possible, we need to verify that the move is actually an en passant capture (should not happen if move generation is correct)
        Square captured_pawn_square = to + (8 * (2 * board->side_to_move - 1)); // to - 8 for white, to + 8 for black
        if (cboard_get_piece_at_square(board, to) != NO_PIECE || cboard_get_piece_at_square(board, captured_pawn_square) != PAWN) {
            // Invalid en passant move, treat as quiet move (should not happen if move generation is correct)
            return make_quiet_move(board, move);
        }
        return make_ep_capture_move(board, move);
    } else if (move_is_promotion(move)) {
        return make_promotion_move(board, move);
    } else // normal move (quiet or capture)
    {
        // check for captures
        if (board->side_to_move == WHITE) {
            if (bitboard_is_bit_set(board->occupancy_bbs[BLACK], to)) {
                return make_capture_move(board, move);
            }
        } else {
            if (bitboard_is_bit_set(board->occupancy_bbs[WHITE], to)) {
                return make_capture_move(board, move);
            }
        }
        // Detect double pawn pushes (used to set en passant square).
        if (moving_piecetype == PAWN) {
            if (board->side_to_move == WHITE) {
                if ((to - from) == 16) {
                    return make_double_pawn_push_move(board, move);
                }
            } else {
                if ((from - to) == 16) {
                    return make_double_pawn_push_move(board, move);
                }
            }
        }
        return make_quiet_move(board, move);
    }
    return (UndoInfo) { 0 }; // Should never reach here
}

static void unmake_promotion_move(CBoard* board, Move move, UndoInfo undo_info)
{
    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);
    // Unmake promotion
    PieceType promotion_piecetype = move_get_promotion_piecetype(move);
    bool is_promotion_capture = (undo_info.captured_piecetype != NO_PIECE);
    cboard_update_occupancies_for_move(board, to, from, board->side_to_move);
    // Replace promoted piece with pawn
    remove_piece_from_cboard(board, to, board->side_to_move, promotion_piecetype);
    add_piece_to_cboard(board, from, board->side_to_move, PAWN);

    // If promotion capture, restore captured_piecetype piece
    if (is_promotion_capture && undo_info.captured_piecetype != NO_PIECE) {
        Color opponent_color = 1 - board->side_to_move;
        add_piece_to_cboard(board, to, opponent_color, undo_info.captured_piecetype);
        if (opponent_color == WHITE)
            bitboard_set_square_bit(&board->occupancy_bbs[WHITE], to);
        else
            bitboard_set_square_bit(&board->occupancy_bbs[BLACK], to);
        bitboard_set_square_bit(&board->occupancy_bbs[2], to);
    }
}

static void unmake_en_passant_move(CBoard* board, Move move, UndoInfo undo_info)
{
    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);
    if (undo_info.captured_piecetype != PAWN) {
        // Invalid undo info for en passant, treat as quiet unmake (should not happen if move generation is correct
        // printf("Warning: Invalid undo info for en passant unmake, treating as quiet unmake\n");
        // Move piece back from destination to source
        move_piece_on_cboard(board, to, from, board->side_to_move);
        cboard_update_occupancies_for_move(board, to, from, board->side_to_move);
        return;
    }
    // Unmake en passant
    // Move pawn back from destination to source
    move_piece_on_cboard(board, to, from, board->side_to_move);
    cboard_update_occupancies_for_move(board, to, from, board->side_to_move);
    // Restore captured_piecetype pawn
    Square captured_pawn_square = to + (8 * (2 * board->side_to_move - 1));

    Color opponent_color = 1 - board->side_to_move;
    add_piece_to_cboard(board, captured_pawn_square, opponent_color, PAWN);
    bitboard_set_square_bit(&board->occupancy_bbs[opponent_color], captured_pawn_square);
    bitboard_set_square_bit(&board->occupancy_bbs[2], captured_pawn_square);
}

static void unmake_castling_move(CBoard* board, Move move)
{
    // Unmake castling

    if (board->side_to_move == WHITE) {
        if (move_get_to_square(move) == G1) {
            // Move king back from g1 to e1 and rook from f1 to h1
            move_piece_on_cboard(board, G1, E1, WHITE);
            move_piece_on_cboard(board, F1, H1, WHITE);
            cboard_update_occupancies_for_castling(board, G1, E1, F1, H1, WHITE);
        } else {
            // Move king back from c1 to e1 and rook from d1 to a1
            move_piece_on_cboard(board, C1, E1, WHITE);
            move_piece_on_cboard(board, D1, A1, WHITE);
            cboard_update_occupancies_for_castling(board, C1, E1, D1, A1, WHITE);
        }
    } else {
        if (move_get_to_square(move) == G8) {
            // Move king back from g8 to e8 and rook from f8 to h8
            move_piece_on_cboard(board, G8, E8, BLACK);
            move_piece_on_cboard(board, F8, H8, BLACK);
            cboard_update_occupancies_for_castling(board, G8, E8, F8, H8, BLACK);
        } else {
            // Move king back from c8 to e8 and rook from d8 to a8
            move_piece_on_cboard(board, C8, E8, BLACK);
            move_piece_on_cboard(board, D8, A8, BLACK);
            cboard_update_occupancies_for_castling(board, C8, E8, D8, A8, BLACK);
        }
    }
}

static void unmake_regular_move(CBoard* board, Move move, UndoInfo undo_info)
{
    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);
    // Unmake regular move (quiet, capture, double pawn push)
    // Move piece back from destination to source
    move_piece_on_cboard(board, to, from, board->side_to_move);
    cboard_update_occupancies_for_move(board, to, from, board->side_to_move);

    // If it was a capture, restore the captured_piecetype piece
    if (undo_info.captured_square != NO_SQUARE && undo_info.captured_piecetype != NO_PIECE) {
        Color opponent_color = 1 - board->side_to_move;
        add_piece_to_cboard(board, to, opponent_color, undo_info.captured_piecetype);
        bitboard_set_square_bit(&board->occupancy_bbs[opponent_color], to);
        bitboard_set_square_bit(&board->occupancy_bbs[2], to);
    }
}

void unmake_move(CBoard* board, Move move, UndoInfo undo_info)
{
    // Square from = move_get_from_square(move);
    // Square to = move_get_to_square(move);

    // Switch side back first (before any checks that depend on sideToMove)
    board->side_to_move = 1 - board->side_to_move;

    // Restore game state
    board->ep_square = undo_info.previous_ep_square;
    board->half_move_clock = undo_info.previous_halfmove_clock;
    board->castling_rights = undo_info.previous_castling_rights;
    board->zobrist_key = undo_info.previous_zobrist_key;

    // Decrement fullmove number if unmaking black's move
    if (board->side_to_move == BLACK) {
        board->full_move_number--;
    }

    // Handle different move types
    if (move_is_castling(move)) {
        unmake_castling_move(board, move);
    } else if (move_is_promotion(move)) {
        unmake_promotion_move(board, move, undo_info);
    } else if (move_is_enpassant(move)) {
        unmake_en_passant_move(board, move, undo_info);
    } else {
        unmake_regular_move(board, move, undo_info);
    }
}
