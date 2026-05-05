
#include "board/cboard.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board/zobrist.h"
#include "core/bitboard.h"

void print_cboard(CBoard* board)
{
    if (!board) {
        printf("Board is NULL\n");
        return;
    }

    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int square_idx = rank * 8 + file; // Fixed calculation
            char piece_char = '.';
            Bitboard square_mask_bb = bitboard_square_mask(square_idx);
            if (board->piece_bbs[WHITE][PAWN] & square_mask_bb)
                piece_char = 'P';
            else if (board->piece_bbs[WHITE][KNIGHT] & square_mask_bb)
                piece_char = 'N';
            else if (board->piece_bbs[WHITE][BISHOP] & square_mask_bb)
                piece_char = 'B';
            else if (board->piece_bbs[WHITE][ROOK] & square_mask_bb)
                piece_char = 'R';
            else if (board->piece_bbs[WHITE][QUEEN] & square_mask_bb)
                piece_char = 'Q';
            else if (board->piece_bbs[WHITE][KING] & square_mask_bb)
                piece_char = 'K';
            else if (board->piece_bbs[BLACK][PAWN] & square_mask_bb)
                piece_char = 'p';
            else if (board->piece_bbs[BLACK][KNIGHT] & square_mask_bb)
                piece_char = 'n';
            else if (board->piece_bbs[BLACK][BISHOP] & square_mask_bb)
                piece_char = 'b';
            else if (board->piece_bbs[BLACK][ROOK] & square_mask_bb)
                piece_char = 'r';
            else if (board->piece_bbs[BLACK][QUEEN] & square_mask_bb)
                piece_char = 'q';
            else if (board->piece_bbs[BLACK][KING] & square_mask_bb)
                piece_char = 'k';
            printf("%c ", piece_char);
        }
        printf("\n"); // Add newline after each rank
    }
    printf("\n");
    printf("White to move: %s\n", board->side_to_move == WHITE ? "Yes" : "No");
    printf("En passant square: %d\n", board->ep_square);
    printf("Halfmove clock: %d\n", board->half_move_clock);
    printf("Fullmove number: %d\n", board->full_move_number);
    printf("Castling rights: %s%s%s%s\n",
        CHECK_BIT(board->castling_rights, 3) ? "K" : "",
        CHECK_BIT(board->castling_rights, 2) ? "Q" : "",
        CHECK_BIT(board->castling_rights, 1) ? "k" : "",
        CHECK_BIT(board->castling_rights, 0) ? "q" : "");
    printf("Zobrist Key: %llu\n", board->zobrist_key);
}

bool fen_string_to_cboard(const char* fen_string, CBoard* board)
{
    *board = (CBoard) { 0 };
    board->ep_square = NO_SQUARE; // default no en passant
    for (int sq = 0; sq < 64; ++sq) {
        board->piece_at_square[sq] = NO_PIECE;
    }
    size_t len = strlen(fen_string);
    int rank = 7;
    int file = 0;
    // get piece placement
    for (size_t i = 0; i < len && rank >= 0; i++) {
        char ch = fen_string[i];
        if (ch == ' ') {
            break; // End of piece placement section
        }
        if (ch == '/') {
            rank--;
            if (file != 8) {
                return false; // Not enough files in this rank
            }
            file = 0;
            if (rank < 0) {
                return false; // More ranks than expected
            }
            continue;
        }
        if (ch >= '1' && ch <= '8') {
            file += (ch - '0');
            continue;
        }
        int square_idx = rank * 8 + file;
        Bitboard square_mask_bb = bitboard_square_mask(square_idx);
        switch (ch) {
        case 'P':
            board->piece_bbs[WHITE][PAWN] |= square_mask_bb;
            board->occupancy_bbs[WHITE] |= square_mask_bb;
            board->piece_at_square[square_idx] = PAWN;
            break;
        case 'N':
            board->piece_bbs[WHITE][KNIGHT] |= square_mask_bb;
            board->occupancy_bbs[WHITE] |= square_mask_bb;
            board->piece_at_square[square_idx] = KNIGHT;
            break;
        case 'B':
            board->piece_bbs[WHITE][BISHOP] |= square_mask_bb;
            board->occupancy_bbs[WHITE] |= square_mask_bb;
            board->piece_at_square[square_idx] = BISHOP;
            break;
        case 'R':
            board->piece_bbs[WHITE][ROOK] |= square_mask_bb;
            board->occupancy_bbs[WHITE] |= square_mask_bb;
            board->piece_at_square[square_idx] = ROOK;
            break;
        case 'Q':
            board->piece_bbs[WHITE][QUEEN] |= square_mask_bb;
            board->occupancy_bbs[WHITE] |= square_mask_bb;
            board->piece_at_square[square_idx] = QUEEN;
            break;
        case 'K':
            board->piece_bbs[WHITE][KING] |= square_mask_bb;
            board->occupancy_bbs[WHITE] |= square_mask_bb;
            board->piece_at_square[square_idx] = KING;
            break;
        case 'p':
            board->piece_bbs[BLACK][PAWN] |= square_mask_bb;
            board->occupancy_bbs[BLACK] |= square_mask_bb;
            board->piece_at_square[square_idx] = PAWN;
            break;
        case 'n':
            board->piece_bbs[BLACK][KNIGHT] |= square_mask_bb;
            board->occupancy_bbs[BLACK] |= square_mask_bb;
            board->piece_at_square[square_idx] = KNIGHT;
            break;
        case 'b':
            board->piece_bbs[BLACK][BISHOP] |= square_mask_bb;
            board->occupancy_bbs[BLACK] |= square_mask_bb;
            board->piece_at_square[square_idx] = BISHOP;
            break;
        case 'r':
            board->piece_bbs[BLACK][ROOK] |= square_mask_bb;
            board->occupancy_bbs[BLACK] |= square_mask_bb;
            board->piece_at_square[square_idx] = ROOK;
            break;
        case 'q':
            board->piece_bbs[BLACK][QUEEN] |= square_mask_bb;
            board->occupancy_bbs[BLACK] |= square_mask_bb;
            board->piece_at_square[square_idx] = QUEEN;
            break;
        case 'k':
            board->piece_bbs[BLACK][KING] |= square_mask_bb;
            board->occupancy_bbs[BLACK] |= square_mask_bb;
            board->piece_at_square[square_idx] = KING;
            break;
        default:
            return false; // Invalid character in piece placement
        }

        file++;
    }
    if (rank < 0) {
        return false; // More ranks than expected
    }
    if (file > 8) {
        return false;
    }

    board->occupancy_bbs[2] = board->occupancy_bbs[WHITE] | board->occupancy_bbs[BLACK];

    // now parse remaining fields safely using strtok-like navigation
    const char* p = strchr(fen_string, ' ');
    if (!p)
        return false;
    ++p;

    // side to move
    if (*p != 'w' && *p != 'b')
        return false; // Invalid side to move character
    board->side_to_move = (*p == 'w') ? WHITE : BLACK;
    p = strchr(p, ' ');
    if (!p)
        return false;
    ++p;

    // castling rights
    if (*p == '-') {
        // no castling
        ++p;
    } else {
        while (*p && *p != ' ') {
            switch (*p) {
            case 'K':
                SET_BIT(board->castling_rights, 3);
                break;
            case 'Q':
                SET_BIT(board->castling_rights, 2);
                break;
            case 'k':
                SET_BIT(board->castling_rights, 1);
                break;
            case 'q':
                SET_BIT(board->castling_rights, 0);
                break;
            default:
                return false; // Invalid castling right character
            }
            ++p;
        }
    }
    if (*p == ' ')
        ++p;

    // en passant
    if (*p && *p != '-') {
        char f = *p; // file letter 'a'..'h'
        char r = *(p + 1); // rank char '1'..'8'
        if (f >= 'a' && f <= 'h' && r >= '1' && r <= '8') {
            int ep_file = f - 'a';
            int ep_rank = r - '1';
            board->ep_square = ep_rank * 8 + ep_file;
        } else {
            return false; // Invalid en passant square
        }
    } else {
        board->ep_square = NO_SQUARE; // no en passant
    }
    // advance to halfmove/fullmove fields
    p = strchr(p, ' ');
    if (p) {
        ++p;
        board->half_move_clock = (uint16_t)atoi(p);
        if (board->half_move_clock < 0) {
            return false; // Halfmove clock cannot be negative
        }
        p = strchr(p, ' ');
        if (p) {
            ++p;
            board->full_move_number = (uint16_t)atoi(p);
            if (board->full_move_number < 1) {
                return false; // Fullmove number must be at least 1
            }
        }
    }
    // ensure exactly one white king and one black king on board
    if (bitboard_popcount(board->piece_bbs[WHITE][KING]) != 1 || bitboard_popcount(board->piece_bbs[BLACK][KING]) != 1) {
        return false; // Invalid number of kings
    }
    // ensure no pawns on first or last rank
    if ((board->piece_bbs[WHITE][PAWN] & (RANK_1 | RANK_8)) || (board->piece_bbs[BLACK][PAWN] & (RANK_1 | RANK_8))) {
        return false; // Pawns cannot be on first or last rank
    }

    compute_zobrist_key(board);
    return true;
}

char* cboard_to_fen(CBoard* board)
{
    char* fen_string = (char*)malloc(128); // enough space

    char* p = fen_string;
    for (int rank = 7; rank >= 0; --rank) {
        int rank_empty_count = 0; // reset per rank
        for (int file = 0; file < 8; ++file) {
            int square_idx = rank * 8 + file;
            Bitboard square_mask_bb = bitboard_square_mask(square_idx);
            char piece_char = '\0';
            if (board->piece_bbs[WHITE][PAWN] & square_mask_bb)
                piece_char = 'P';
            else if (board->piece_bbs[WHITE][KNIGHT] & square_mask_bb)
                piece_char = 'N';
            else if (board->piece_bbs[WHITE][BISHOP] & square_mask_bb)
                piece_char = 'B';
            else if (board->piece_bbs[WHITE][ROOK] & square_mask_bb)
                piece_char = 'R';
            else if (board->piece_bbs[WHITE][QUEEN] & square_mask_bb)
                piece_char = 'Q';
            else if (board->piece_bbs[WHITE][KING] & square_mask_bb)
                piece_char = 'K';
            else if (board->piece_bbs[BLACK][PAWN] & square_mask_bb)
                piece_char = 'p';
            else if (board->piece_bbs[BLACK][KNIGHT] & square_mask_bb)
                piece_char = 'n';
            else if (board->piece_bbs[BLACK][BISHOP] & square_mask_bb)
                piece_char = 'b';
            else if (board->piece_bbs[BLACK][ROOK] & square_mask_bb)
                piece_char = 'r';
            else if (board->piece_bbs[BLACK][QUEEN] & square_mask_bb)
                piece_char = 'q';
            else if (board->piece_bbs[BLACK][KING] & square_mask_bb)
                piece_char = 'k';

            if (piece_char) {
                if (rank_empty_count > 0) {
                    p += sprintf(p, "%d", rank_empty_count);
                    rank_empty_count = 0;
                }
                *p++ = piece_char;
            } else {
                ++rank_empty_count; // increment for empty square
            }
        }

        if (rank_empty_count > 0)
            p += sprintf(p, "%d", rank_empty_count);

        if (rank > 0)
            *p++ = '/';
    }

    *p++ = ' ';

    // side to move
    *p++ = board->side_to_move == WHITE ? 'w' : 'b';
    *p++ = ' ';

    // castling rights
    bool any = false;
    if (CHECK_BIT(board->castling_rights, 3)) {
        *p++ = 'K';
        any = true;
    }
    if (CHECK_BIT(board->castling_rights, 2)) {
        *p++ = 'Q';
        any = true;
    }
    if (CHECK_BIT(board->castling_rights, 1)) {
        *p++ = 'k';
        any = true;
    }
    if (CHECK_BIT(board->castling_rights, 0)) {
        *p++ = 'q';
        any = true;
    }
    if (!any)
        *p++ = '-';
    *p++ = ' ';

    // en passant
    if (board->ep_square != NO_SQUARE) {
        int ep_file = board->ep_square % 8;
        int ep_rank = board->ep_square / 8;
        *p++ = 'a' + ep_file;
        *p++ = '1' + ep_rank;
    } else {
        *p++ = '-';
    }
    *p++ = ' ';

    // halfmove clock and fullmove number
    p += sprintf(p, "%u %u", board->half_move_clock, board->full_move_number);

    *p = '\0';
    return fen_string;
}

PieceType cboard_get_piece_at_square(const CBoard* board, Square square)
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

Bitboard* cboard_get_piece_bitboard(CBoard* board, Color color, PieceType piece)
{
    if (color == WHITE) {
        switch (piece) {
        case PAWN:
            return &board->piece_bbs[WHITE][PAWN];
        case KNIGHT:
            return &board->piece_bbs[WHITE][KNIGHT];
        case BISHOP:
            return &board->piece_bbs[WHITE][BISHOP];
        case ROOK:
            return &board->piece_bbs[WHITE][ROOK];
        case QUEEN:
            return &board->piece_bbs[WHITE][QUEEN];
        case KING:
            return &board->piece_bbs[WHITE][KING];
        default:
            return NULL;
        }
    } else {
        switch (piece) {
        case PAWN:
            return &board->piece_bbs[BLACK][PAWN];
        case KNIGHT:
            return &board->piece_bbs[BLACK][KNIGHT];
        case BISHOP:
            return &board->piece_bbs[BLACK][BISHOP];
        case ROOK:
            return &board->piece_bbs[BLACK][ROOK];
        case QUEEN:
            return &board->piece_bbs[BLACK][QUEEN];
        case KING:
            return &board->piece_bbs[BLACK][KING];
        default:
            return NULL;
        }
    }
}

void add_piece_to_cboard(CBoard* board, Square square, Color color, PieceType piece)
{
    Bitboard* bb = cboard_get_piece_bitboard(board, color, piece);
    if (!bb) {
        return;
    }

    bitboard_set_square_bit(bb, square);
    board->piece_at_square[square] = piece;
}

void remove_piece_from_cboard(CBoard* board, Square square, Color color, PieceType piece)
{
    Bitboard* bb = cboard_get_piece_bitboard(board, color, piece);
    if (!bb) {
        return;
    }

    bitboard_clear_square_bit(bb, square);
    board->piece_at_square[square] = NO_PIECE;
}

void move_piece_on_cboard(CBoard* board, Square from, Square to, Color side)
{
    PieceType moving_piece = cboard_get_piece_at_square(board, from);
    if (moving_piece == NO_PIECE) {
        return;
    }

    remove_piece_from_cboard(board, from, side, moving_piece);
    add_piece_to_cboard(board, to, side, moving_piece);
}

PieceType cboard_remove_captured_piece(CBoard* board, Square square, Color capturingColor)
{
    Color captured_color = (capturingColor == WHITE) ? BLACK : WHITE;

    PieceType captured_piece = cboard_get_piece_at_square(board, square);
    if (captured_piece == NO_PIECE) {
        return NO_PIECE;
    }

    remove_piece_from_cboard(board, square, captured_color, captured_piece);
    return captured_piece;
}

void cboard_update_occupancies_for_move(CBoard* board, Square from, Square to, Color color)
{
    bitboard_clear_square_bit(&board->occupancy_bbs[color], from);
    bitboard_set_square_bit(&board->occupancy_bbs[color], to);
    bitboard_clear_square_bit(&board->occupancy_bbs[2], from);
    bitboard_set_square_bit(&board->occupancy_bbs[2], to);
}

void cboard_update_occupancies_for_capture(CBoard* board, Square square, Color captured_color)
{
    bitboard_clear_square_bit(&board->occupancy_bbs[captured_color], square);
    bitboard_clear_square_bit(&board->occupancy_bbs[2], square);
}

void cboard_update_occupancies_for_promotion(CBoard* board, Square from, Square to, Color color)
{
    cboard_update_occupancies_for_move(board, from, to, color);
}

void cboard_update_occupancies_for_castling(CBoard* board, Square king_from, Square king_to,
    Square rook_from, Square rook_to, Color color)
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

void cboard_update_castling_rights(CBoard* board, Square from, Square to)
{
    // If king moved, lose all castling
    if (bitboard_is_bit_set(board->piece_bbs[WHITE][KING], to)) {
        CLEAR_BIT(board->castling_rights, 3);
        CLEAR_BIT(board->castling_rights, 2);
    } else if (bitboard_is_bit_set(board->piece_bbs[BLACK][KING], to)) {
        CLEAR_BIT(board->castling_rights, 1);
        CLEAR_BIT(board->castling_rights, 0);
    }

    // If rook moved from corner, lose that side's castling
    if (from == H1)
        CLEAR_BIT(board->castling_rights, 3);
    if (from == A1)
        CLEAR_BIT(board->castling_rights, 2);
    if (from == H8)
        CLEAR_BIT(board->castling_rights, 1);
    if (from == A8)
        CLEAR_BIT(board->castling_rights, 0);

    // If rook was captured on corner square, lose that side's castling
    if (to == H1)
        CLEAR_BIT(board->castling_rights, 3);
    if (to == A1)
        CLEAR_BIT(board->castling_rights, 2);
    if (to == H8)
        CLEAR_BIT(board->castling_rights, 1);
    if (to == A8)
        CLEAR_BIT(board->castling_rights, 0);
}