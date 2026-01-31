// printMove(Move) - print move in coordinate notation (e2e4)
#include "testing_utils.h"

char *moveToStringCoordinate(Move move)
{
    static char str[5];
    char files[] = "abcdefgh";
    char ranks[] = "12345678";
    str[0] = files[move.from % 8];
    str[1] = ranks[move.from / 8];
    str[2] = files[move.to % 8];
    str[3] = ranks[move.to / 8];
    str[4] = '\0';
    return str;
}

char *moveToStringAlgebraic(Move move)
{
    static char moveStr[10];
    const char *files = "abcdefgh";
    const char *ranks = "12345678";

    Square from = FROM_SQ(move);
    Square to = TO_SQ(move);
    MoveFlag flag = MOVE_FLAG(move);

    int fromFile = from % 8;
    int fromRank = from / 8;
    int toFile = to % 8;
    int toRank = to / 8;

    // Basic move notation: e2e4
    sprintf(moveStr, "%c%c%c%c",
            files[fromFile], ranks[fromRank],
            files[toFile], ranks[toRank]);

    // Add promotion piece if applicable
    if (flag >= KNIGHT_PROMO_QUIET && flag <= QUEEN_PROMO_CAPTURE)
    {
        char promoChar = ' ';
        switch (flag)
        {
        case KNIGHT_PROMO_QUIET:
        case KNIGHT_PROMO_CAPTURE:
            promoChar = 'n';
            break;
        case BISHOP_PROMO_QUIET:
        case BISHOP_PROMO_CAPTURE:
            promoChar = 'b';
            break;
        case ROOK_PROMO_QUIET:
        case ROOK_PROMO_CAPTURE:
            promoChar = 'r';
            break;
        case QUEEN_PROMO_QUIET:
        case QUEEN_PROMO_CAPTURE:
            promoChar = 'q';
            break;
        default:
            break;
        }
        sprintf(moveStr + 4, "%c", promoChar);
    }

    return moveStr;
}