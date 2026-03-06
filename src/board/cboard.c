
#include "board/cboard.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/bitboard.h"
#include "board/zobrist.h"

void printBoard(CBoard *board)
{
    if (!board)
    {
        printf("Board is NULL\n");
        return;
    }

    for (int rank = 7; rank >= 0; rank--)
    {
        for (int file = 0; file < 8; file++)
        {
            int squareIndex = rank * 8 + file; // Fixed calculation
            char pieceChar = '.';
            Bitboard squareMask = bitboardSquareMask(squareIndex);
            if (board->whitePawns & squareMask)
                pieceChar = 'P';
            else if (board->whiteKnights & squareMask)
                pieceChar = 'N';
            else if (board->whiteBishops & squareMask)
                pieceChar = 'B';
            else if (board->whiteRooks & squareMask)
                pieceChar = 'R';
            else if (board->whiteQueens & squareMask)
                pieceChar = 'Q';
            else if (board->whiteKing & squareMask)
                pieceChar = 'K';
            else if (board->blackPawns & squareMask)
                pieceChar = 'p';
            else if (board->blackKnights & squareMask)
                pieceChar = 'n';
            else if (board->blackBishops & squareMask)
                pieceChar = 'b';
            else if (board->blackRooks & squareMask)
                pieceChar = 'r';
            else if (board->blackQueens & squareMask)
                pieceChar = 'q';
            else if (board->blackKing & squareMask)
                pieceChar = 'k';
            printf("%c ", pieceChar);
        }
        printf("\n"); // Add newline after each rank
    }
    printf("\n");
    printf("White to move: %s\n", board->sideToMove == WHITE ? "Yes" : "No");
    printf("En passant square: %d\n", board->epSquare);
    printf("Halfmove clock: %d\n", board->halfmoveClock);
    printf("Fullmove number: %d\n", board->fullmoveNumber);
    printf("Castling rights: %s%s%s%s\n",
           CHECK_BIT(board->castlingRights, 3) ? "K" : "",
           CHECK_BIT(board->castlingRights, 2) ? "Q" : "",
           CHECK_BIT(board->castlingRights, 1) ? "k" : "",
           CHECK_BIT(board->castlingRights, 0) ? "q" : "");
    printf("Zobrist Key: %llu\n", board->zobristKey);
}

bool fenToCBoard(const char *fenString, CBoard *board) // TODO: Add error handling for invalid FEN strings
{
    *board = (CBoard){0};
    board->epSquare = NO_SQUARE; // default no en passant
    for (int sq = 0; sq < 64; ++sq)
    {
        board->pieceAtSquare[sq] = NO_PIECE;
    }
    size_t len = strlen(fenString);
    int rank = 7;
    int file = 0;
    // get piece placement
    for (size_t i = 0; i < len && rank >= 0; i++)
    {
        char ch = fenString[i];
        if (ch == ' ')
        {
            break; // End of piece placement section
        }
        if (ch == '/')
        {
            rank--;
            if (file != 8)
            {
                return false; // Not enough files in this rank
            }
            file = 0;
            if (rank < 0)
            {
                return false; // More ranks than expected
            }
            continue;
        }
        if (ch >= '1' && ch <= '8')
        {
            file += (ch - '0');
            continue;
        }
        int squareIndex = rank * 8 + file;
        Bitboard squareMask = bitboardSquareMask(squareIndex);
        switch (ch)
        {
        case 'P':
            board->whitePawns |= squareMask;
            board->whitePieces |= squareMask;
            board->pieceAtSquare[squareIndex] = PAWN;
            break;
        case 'N':
            board->whiteKnights |= squareMask;
            board->whitePieces |= squareMask;
            board->pieceAtSquare[squareIndex] = KNIGHT;
            break;
        case 'B':
            board->whiteBishops |= squareMask;
            board->whitePieces |= squareMask;
            board->pieceAtSquare[squareIndex] = BISHOP;
            break;
        case 'R':
            board->whiteRooks |= squareMask;
            board->whitePieces |= squareMask;
            board->pieceAtSquare[squareIndex] = ROOK;
            break;
        case 'Q':
            board->whiteQueens |= squareMask;
            board->whitePieces |= squareMask;
            board->pieceAtSquare[squareIndex] = QUEEN;
            break;
        case 'K':
            board->whiteKing |= squareMask;
            board->whitePieces |= squareMask;
            board->pieceAtSquare[squareIndex] = KING;
            break;
        case 'p':
            board->blackPawns |= squareMask;
            board->blackPieces |= squareMask;
            board->pieceAtSquare[squareIndex] = PAWN;
            break;
        case 'n':
            board->blackKnights |= squareMask;
            board->blackPieces |= squareMask;
            board->pieceAtSquare[squareIndex] = KNIGHT;
            break;
        case 'b':
            board->blackBishops |= squareMask;
            board->blackPieces |= squareMask;
            board->pieceAtSquare[squareIndex] = BISHOP;
            break;
        case 'r':
            board->blackRooks |= squareMask;
            board->blackPieces |= squareMask;
            board->pieceAtSquare[squareIndex] = ROOK;
            break;
        case 'q':
            board->blackQueens |= squareMask;
            board->blackPieces |= squareMask;
            board->pieceAtSquare[squareIndex] = QUEEN;
            break;
        case 'k':
            board->blackKing |= squareMask;
            board->blackPieces |= squareMask;
            board->pieceAtSquare[squareIndex] = KING;
            break;
        default:
            return false; // Invalid character in piece placement
        }

        file++;
    }
    if (rank < 0)
    {
        return false; // More ranks than expected
    }
    if (file > 8)
    {
        return false;
    }

    board->allPieces = board->whitePieces | board->blackPieces;

    // now parse remaining fields safely using strtok-like navigation
    const char *p = strchr(fenString, ' ');
    if (!p)
        return false;
    ++p;

    // side to move
    if (*p != 'w' && *p != 'b')
        return false; // Invalid side to move character
    board->sideToMove = (*p == 'w') ? WHITE : BLACK;
    p = strchr(p, ' ');
    if (!p)
        return false;
    ++p;

    // castling rights
    if (*p == '-')
    {
        // no castling
        ++p;
    }
    else
    {
        while (*p && *p != ' ')
        {
            switch (*p)
            {
            case 'K':
                SET_BIT(board->castlingRights, 3);
                break;
            case 'Q':
                SET_BIT(board->castlingRights, 2);
                break;
            case 'k':
                SET_BIT(board->castlingRights, 1);
                break;
            case 'q':
                SET_BIT(board->castlingRights, 0);
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
    if (*p && *p != '-')
    {
        char f = *p;       // file letter 'a'..'h'
        char r = *(p + 1); // rank char '1'..'8'
        if (f >= 'a' && f <= 'h' && r >= '1' && r <= '8')
        {
            int epFile = f - 'a';
            int epRank = r - '1';
            board->epSquare = epRank * 8 + epFile;
        }
        else
        {
            return false; // Invalid en passant square
        }
    }
    else
    {
        board->epSquare = NO_SQUARE; // no en passant
    }
    // advance to halfmove/fullmove fields
    p = strchr(p, ' ');
    if (p)
    {
        ++p;
        board->halfmoveClock = (uint16_t)atoi(p);
        if (board->halfmoveClock < 0)
        {
            return false; // Halfmove clock cannot be negative
        }
        p = strchr(p, ' ');
        if (p)
        {
            ++p;
            board->fullmoveNumber = (uint16_t)atoi(p);
            if (board->fullmoveNumber < 1)
            {
                return false; // Fullmove number must be at least 1
            }
        }
    }
    // ensure exactly one white king and one black king on board
    if (bitBoardPopcount(board->whiteKing) != 1 || bitBoardPopcount(board->blackKing) != 1)
    {
        return false; // Invalid number of kings
    }
    // ensure no pawns on first or last rank
    if ((board->whitePawns & (RANK_1 | RANK_8)) || (board->blackPawns & (RANK_1 | RANK_8)))
    {
        return false; // Pawns cannot be on first or last rank
    }

    computeZobristKey(board);
    return true;
}

char *CBoardToFen(CBoard *board)
{
    char *fenString = (char *)malloc(128); // enough space

    char *p = fenString;
    for (int rank = 7; rank >= 0; --rank)
    {
        int emptyCount = 0; // reset per rank
        for (int file = 0; file < 8; ++file)
        {
            int squareIndex = rank * 8 + file;
            Bitboard squareMask = bitboardSquareMask(squareIndex);
            char pieceChar = '\0';
            if (board->whitePawns & squareMask)
                pieceChar = 'P';
            else if (board->whiteKnights & squareMask)
                pieceChar = 'N';
            else if (board->whiteBishops & squareMask)
                pieceChar = 'B';
            else if (board->whiteRooks & squareMask)
                pieceChar = 'R';
            else if (board->whiteQueens & squareMask)
                pieceChar = 'Q';
            else if (board->whiteKing & squareMask)
                pieceChar = 'K';
            else if (board->blackPawns & squareMask)
                pieceChar = 'p';
            else if (board->blackKnights & squareMask)
                pieceChar = 'n';
            else if (board->blackBishops & squareMask)
                pieceChar = 'b';
            else if (board->blackRooks & squareMask)
                pieceChar = 'r';
            else if (board->blackQueens & squareMask)
                pieceChar = 'q';
            else if (board->blackKing & squareMask)
                pieceChar = 'k';

            if (pieceChar)
            {
                if (emptyCount > 0)
                {
                    p += sprintf(p, "%d", emptyCount);
                    emptyCount = 0;
                }
                *p++ = pieceChar;
            }
            else
            {
                ++emptyCount; // increment for empty square
            }
        }

        if (emptyCount > 0)
            p += sprintf(p, "%d", emptyCount);

        if (rank > 0)
            *p++ = '/';
    }

    *p++ = ' ';

    // side to move
    *p++ = board->sideToMove == WHITE ? 'w' : 'b';
    *p++ = ' ';

    // castling rights
    bool any = false;
    if (CHECK_BIT(board->castlingRights, 3))
    {
        *p++ = 'K';
        any = true;
    }
    if (CHECK_BIT(board->castlingRights, 2))
    {
        *p++ = 'Q';
        any = true;
    }
    if (CHECK_BIT(board->castlingRights, 1))
    {
        *p++ = 'k';
        any = true;
    }
    if (CHECK_BIT(board->castlingRights, 0))
    {
        *p++ = 'q';
        any = true;
    }
    if (!any)
        *p++ = '-';
    *p++ = ' ';

    // en passant
    if (board->epSquare != NO_SQUARE)
    {
        int epFile = board->epSquare % 8;
        int epRank = board->epSquare / 8;
        *p++ = 'a' + epFile;
        *p++ = '1' + epRank;
    }
    else
    {
        *p++ = '-';
    }
    *p++ = ' ';

    // halfmove clock and fullmove number
    p += sprintf(p, "%u %u", board->halfmoveClock, board->fullmoveNumber);

    *p = '\0';
    return fenString;
}

PieceType getPieceAtSquare(const CBoard *board, Square square)
{
    if (!board || square < A1 || square >= NO_SQUARE)
    {
        return NO_PIECE;
    }

    // Mailbox-backed lookup with occupancy validation.
    if (!bitboardIsBitSet(board->allPieces, square))
    {
        return NO_PIECE;
    }

    return board->pieceAtSquare[square];
}

Bitboard *pieceBitboard(CBoard *board, Color color, PieceType piece)
{
    if (color == WHITE)
    {
        switch (piece)
        {
        case PAWN:
            return &board->whitePawns;
        case KNIGHT:
            return &board->whiteKnights;
        case BISHOP:
            return &board->whiteBishops;
        case ROOK:
            return &board->whiteRooks;
        case QUEEN:
            return &board->whiteQueens;
        case KING:
            return &board->whiteKing;
        default:
            return NULL;
        }
    }
    else
    {
        switch (piece)
        {
        case PAWN:
            return &board->blackPawns;
        case KNIGHT:
            return &board->blackKnights;
        case BISHOP:
            return &board->blackBishops;
        case ROOK:
            return &board->blackRooks;
        case QUEEN:
            return &board->blackQueens;
        case KING:
            return &board->blackKing;
        default:
            return NULL;
        }
    }
}

void addPieceToBoard(CBoard *board, Square square, Color color, PieceType piece)
{
    Bitboard *bb = pieceBitboard(board, color, piece);
    if (!bb)
    {
        return;
    }

    bitboardSetSquareBit(bb, square);
    board->pieceAtSquare[square] = piece;
}

void removePieceFromBoard(CBoard *board, Square square, Color color, PieceType piece)
{
    Bitboard *bb = pieceBitboard(board, color, piece);
    if (!bb)
    {
        return;
    }

    bitboardClearSquareBit(bb, square);
    board->pieceAtSquare[square] = NO_PIECE;
}

void movePieceOnBoard(CBoard *board, Square from, Square to, Color side)
{
    PieceType movingPiece = getPieceAtSquare(board, from);
    if (movingPiece == NO_PIECE)
    {
        return;
    }

    removePieceFromBoard(board, from, side, movingPiece);
    addPieceToBoard(board, to, side, movingPiece);
}

PieceType removeCapturedPiece(CBoard *board, Square square, Color capturingColor)
{
    Color capturedColor = (capturingColor == WHITE) ? BLACK : WHITE;

    PieceType capturedPiece = getPieceAtSquare(board, square);
    if (capturedPiece == NO_PIECE)
    {
        return NO_PIECE;
    }

    removePieceFromBoard(board, square, capturedColor, capturedPiece);
    return capturedPiece;
}

void updateOccupanciesForMove(CBoard *board, Square from, Square to, Color color)
{
    if (color == WHITE)
    {
        bitboardClearSquareBit(&board->whitePieces, from);
        bitboardSetSquareBit(&board->whitePieces, to);
    }
    else
    {
        bitboardClearSquareBit(&board->blackPieces, from);
        bitboardSetSquareBit(&board->blackPieces, to);
    }
    bitboardClearSquareBit(&board->allPieces, from);
    bitboardSetSquareBit(&board->allPieces, to);
}

void updateOccupanciesForCapture(CBoard *board, Square square, Color capturedColor)
{
    if (capturedColor == WHITE)
    {
        bitboardClearSquareBit(&board->whitePieces, square);
    }
    else
    {
        bitboardClearSquareBit(&board->blackPieces, square);
    }
    bitboardClearSquareBit(&board->allPieces, square);
}

void updateOccupanciesForPromotion(CBoard *board, Square from, Square to, Color color)
{
    updateOccupanciesForMove(board, from, to, color);
}

void updateOccupanciesForCastling(CBoard *board, Square kingFrom, Square kingTo,
                                  Square rookFrom, Square rookTo, Color color)
{
    if (color == WHITE)
    {
        bitboardClearSquareBit(&board->whitePieces, kingFrom);
        bitboardClearSquareBit(&board->whitePieces, rookFrom);
        bitboardSetSquareBit(&board->whitePieces, kingTo);
        bitboardSetSquareBit(&board->whitePieces, rookTo);
    }
    else
    {
        bitboardClearSquareBit(&board->blackPieces, kingFrom);
        bitboardClearSquareBit(&board->blackPieces, rookFrom);
        bitboardSetSquareBit(&board->blackPieces, kingTo);
        bitboardSetSquareBit(&board->blackPieces, rookTo);
    }
    bitboardClearSquareBit(&board->allPieces, kingFrom);
    bitboardClearSquareBit(&board->allPieces, rookFrom);
    bitboardSetSquareBit(&board->allPieces, kingTo);
    bitboardSetSquareBit(&board->allPieces, rookTo);
}

void updateCastlingRights(CBoard *board, Square from, Square to)
{
    // If king moved, lose all castling
    if (bitboardIsBitSet(board->whiteKing, to))
    {
        CLEAR_BIT(board->castlingRights, 3);
        CLEAR_BIT(board->castlingRights, 2);
        // board->whiteCanCastleQueenside = false;
    }
    else if (bitboardIsBitSet(board->blackKing, to))
    {
        CLEAR_BIT(board->castlingRights, 1);
        CLEAR_BIT(board->castlingRights, 0);
        // board->blackCanCastleKingside = false;
        // board->blackCanCastleQueenside = false;
    }

    // If rook moved from corner, lose that side's castling
    if (from == H1)
        CLEAR_BIT(board->castlingRights, 3);
    if (from == A1)
        CLEAR_BIT(board->castlingRights, 2);
    if (from == H8)
        CLEAR_BIT(board->castlingRights, 1);
    if (from == A8)
        CLEAR_BIT(board->castlingRights, 0);

    // If rook was captured on corner square, lose that side's castling
    if (to == H1)
        CLEAR_BIT(board->castlingRights, 3);
    if (to == A1)
        CLEAR_BIT(board->castlingRights, 2);
    if (to == H8)
        CLEAR_BIT(board->castlingRights, 1);
    if (to == A8)
        CLEAR_BIT(board->castlingRights, 0);
}