
#include "board/cboard.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/bitboard.h"
#include "board/zobrist.h"

// Recompute occupancy bitboards based on individual piece bitboards
void recomputeOccupancies(CBoard *board)
{
    board->whitePieces = board->whiteBishops | board->whiteKing | board->whiteKnights | board->whitePawns | board->whiteQueens | board->whiteRooks; // white pieces are the OR of all white piece bitboards
    board->blackPieces = board->blackBishops | board->blackKing | board->blackKnights | board->blackPawns | board->blackQueens | board->blackRooks; // black pieces are the OR of all black piece bitboards
    board->allPieces = board->whitePieces | board->blackPieces;                                                                                     // all pieces are the OR of white and black pieces
}

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
            Bitboard squareMask = bb_square(squareIndex);
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

// cboard from fen function
CBoard fenToCBoard(const char *fenString)
{
    CBoard board = {0};
    board.epSquare = NO_SQUARE; // default no en passant
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
            file = 0;
            continue;
        }
        if (ch >= '1' && ch <= '8')
        {
            file += (ch - '0');
            continue;
        }
        int squareIndex = rank * 8 + file;
        Bitboard squareMask = bb_square(squareIndex);
        switch (ch)
        {
        case 'P':
            board.whitePawns |= squareMask;
            break;
        case 'N':
            board.whiteKnights |= squareMask;
            break;
        case 'B':
            board.whiteBishops |= squareMask;
            break;
        case 'R':
            board.whiteRooks |= squareMask;
            break;
        case 'Q':
            board.whiteQueens |= squareMask;
            break;
        case 'K':
            board.whiteKing |= squareMask;
            break;
        case 'p':
            board.blackPawns |= squareMask;
            break;
        case 'n':
            board.blackKnights |= squareMask;
            break;
        case 'b':
            board.blackBishops |= squareMask;
            break;
        case 'r':
            board.blackRooks |= squareMask;
            break;
        case 'q':
            board.blackQueens |= squareMask;
            break;
        case 'k':
            board.blackKing |= squareMask;
            break;
        default:
            fputs("Unexpected character in FEN: ", stderr);
            fputc(ch, stderr);
            fputc('\n', stderr);
            break;
        }

        file++;
    }

    // build derived occupancies once
    recomputeOccupancies(&board);

    // now parse remaining fields safely using strtok-like navigation
    const char *p = strchr(fenString, ' ');
    if (!p)
        return board;
    ++p;

    // side to move
    board.sideToMove = (*p == 'w') ? WHITE : BLACK;
    p = strchr(p, ' ');
    if (!p)
        return board;
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
                SET_BIT(board.castlingRights, 3);
                break;
            case 'Q':
                SET_BIT(board.castlingRights, 2);
                break;
            case 'k':
                SET_BIT(board.castlingRights, 1);
                break;
            case 'q':
                SET_BIT(board.castlingRights, 0);
                break;
            default:
                break;
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
            board.epSquare = epRank * 8 + epFile;
        }
        else
        {
            board.epSquare = NO_SQUARE; // invalid en passant square
        }
    }
    else
    {
        board.epSquare = NO_SQUARE; // no en passant
    }
    // advance to halfmove/fullmove fields
    p = strchr(p, ' ');
    if (p)
    {
        ++p;
        board.halfmoveClock = (uint16_t)atoi(p);
        p = strchr(p, ' ');
        if (p)
        {
            ++p;
            board.fullmoveNumber = (uint16_t)atoi(p);
        }
    }
    computeZobristKey(&board);
    return board;
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
            Bitboard squareMask = bb_square(squareIndex);
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