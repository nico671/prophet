#include "hceval.h"

int evaluateBoard(CBoard *board)
{
    int score = 0;

    // White Material (+)
    score += bb_popcount(board->whitePawns) * PAWN_VALUE;
    score += bb_popcount(board->whiteKnights) * KNIGHT_VALUE;
    score += bb_popcount(board->whiteBishops) * BISHOP_VALUE;
    score += bb_popcount(board->whiteRooks) * ROOK_VALUE;
    score += bb_popcount(board->whiteQueens) * QUEEN_VALUE;

    // Black Material (-)
    score -= bb_popcount(board->blackPawns) * PAWN_VALUE;
    score -= bb_popcount(board->blackKnights) * KNIGHT_VALUE;
    score -= bb_popcount(board->blackBishops) * BISHOP_VALUE;
    score -= bb_popcount(board->blackRooks) * ROOK_VALUE;
    score -= bb_popcount(board->blackQueens) * QUEEN_VALUE;

    // Return score from the perspective of the side to move
    // (If it's Black's turn and the score is -300, Black is actually +300)
    return (board->sideToMove == WHITE) ? score : -score;
}