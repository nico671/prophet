// printMove(Move) - print move in coordinate notation (e2e4)
#include "testing_utils.h"

#include <stdio.h>

char* moveToStringCoordinate(Move move)
{
    static char str[5];
    char files[] = "abcdefgh";
    char ranks[] = "12345678";
    str[0] = files[move_get_from_square(move) % 8];
    str[1] = ranks[move_get_from_square(move) / 8];
    str[2] = files[move_get_to_square(move) % 8];
    str[3] = ranks[move_get_to_square(move) / 8];
    str[4] = '\0';
    return str;
}

char* moveToStringAlgebraic(Move move)
{
    static char moveStr[10];
    const char* files = "abcdefgh";
    const char* ranks = "12345678";

    Square from = move_get_from_square(move);
    Square to = move_get_to_square(move);
    MoveType type = move_get_move_type(move);
    PieceType promoPiece = move_get_promotion_piecetype(move);

    int fromFile = from % 8;
    int fromRank = from / 8;
    int toFile = to % 8;
    int toRank = to / 8;

    // Basic move notation: e2e4
    sprintf(moveStr, "%c%c%c%c",
        files[fromFile], ranks[fromRank],
        files[toFile], ranks[toRank]);

    // Add promotion piece if applicable
    if (type == PROMO) {
        char promoChar = ' ';
        switch (promoPiece) {
        case KNIGHT:
            promoChar = 'n';
            break;
        case BISHOP:
            promoChar = 'b';
            break;
        case ROOK:
            promoChar = 'r';
            break;
        case QUEEN:
            promoChar = 'q';
            break;
        default:
            break;
        }
        sprintf(moveStr + 4, "%c", promoChar);
    }

    return moveStr;
}
