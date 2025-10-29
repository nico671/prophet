
#include "cboard.h"

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
            Bitboard squareMask = (Bitboard)1 << squareIndex;
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
    printf("White to move: %s\n", board->whiteToMove ? "Yes" : "No");
    printf("En passant square: %d\n", board->epSquare);
    printf("Halfmove clock: %d\n", board->halfmoveClock);
    printf("Fullmove number: %d\n", board->fullmoveNumber);
}
