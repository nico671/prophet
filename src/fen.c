#include "fen.h"
// cboard from fen function
CBoard fenToCBoard(char *fenString)
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
        Bitboard squareMask = (Bitboard)1 << squareIndex;
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
            board.whiteKingSquare = squareIndex;
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
            board.blackKingSquare = squareIndex;
            break;
        default:
            fprintf(stderr, "Unexpected character in FEN: %c\n", ch);
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
                board.whiteCanCastleKingside = true;
                break;
            case 'Q':
                board.whiteCanCastleQueenside = true;
                break;
            case 'k':
                board.blackCanCastleKingside = true;
                break;
            case 'q':
                board.blackCanCastleQueenside = true;
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
            Bitboard squareMask = (Bitboard)1 << squareIndex;
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
    if (board->whiteCanCastleKingside)
    {
        *p++ = 'K';
        any = true;
    }
    if (board->whiteCanCastleQueenside)
    {
        *p++ = 'Q';
        any = true;
    }
    if (board->blackCanCastleKingside)
    {
        *p++ = 'k';
        any = true;
    }
    if (board->blackCanCastleQueenside)
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