#include "search.h"

int negamax(CBoard *node, int depth, int alpha, int beta, Color color)
{
    // check if depth is 0 or game over
    if (depth == 0)
    {
        return evaluateBoard(node);
    }

    MoveList moveList = generateLegalMoves(node);

    // check for checkmate or stalemate
    if (isKingInCheck(node, color) && moveList.count == 0)
    {
        return -200000000; // Large negative value for checkmate
    }
    if (!isKingInCheck(node, color) && moveList.count == 0)
    {
        return 0; // Draw score for stalemate
    }
    // TODO: implement move ordering for better alpha-beta pruning
    int maxEval = -200000000;
    for (int i = 0; i < moveList.count; i++)
    {
        Move move = moveList.moves[i];

        // Make the move
        UndoInfo undoInfo = makeMove(node, move);

        // Recurse
        int eval = -negamax(node, depth - 1, -beta, -alpha, color == WHITE ? BLACK : WHITE);

        // Unmake the move
        unmakeMove(node, move, undoInfo);

        if (eval > maxEval)
        {
            maxEval = eval;
        }
        if (maxEval > alpha)
        {
            alpha = maxEval;
        }
        if (alpha >= beta)
        {
            break; // beta cutoff
        }
    }

    return maxEval;
}
