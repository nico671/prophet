#include "search/search.h"
#include "hcevaluation/hceval.h"
#include "board/cboard.h"
#include "hcevaluation/hceval.h"
#include "movegen/movegen.h"
#include "movegen/move_make.h"

void scoreMoves(CBoard *board, MoveList *moveList, int *scores)
{
    for (int i = 0; i < moveList->count; i++)
    {
        Move currMove = moveList->moves[i];
        if (move_is_capture(currMove))
        {
            int capturedPieceValue = 0;
            PieceType capturedPiece = NO_PIECE;

            if (bitboardIsBitSet(board->whitePawns, TO_SQ(currMove)) | bitboardIsBitSet(board->blackPawns, TO_SQ(currMove)))
            {
                capturedPiece = PAWN;
            }
            else if (bitboardIsBitSet(board->whiteKnights, TO_SQ(currMove)) || bitboardIsBitSet(board->blackKnights, TO_SQ(currMove)))
            {
                capturedPiece = KNIGHT;
            }
            else if (bitboardIsBitSet(board->whiteBishops, TO_SQ(currMove)) || bitboardIsBitSet(board->blackBishops, TO_SQ(currMove)))
            {
                capturedPiece = BISHOP;
            }
            else if (bitboardIsBitSet(board->whiteRooks, TO_SQ(currMove)) || bitboardIsBitSet(board->blackRooks, TO_SQ(currMove)))
            {
                capturedPiece = ROOK;
            }
            else if (bitboardIsBitSet(board->whiteQueens, TO_SQ(currMove)) || bitboardIsBitSet(board->blackQueens, TO_SQ(currMove)))
            {
                capturedPiece = QUEEN;
            }
            else if (bitboardIsBitSet(board->whiteKing, TO_SQ(currMove)) || bitboardIsBitSet(board->blackKing, TO_SQ(currMove)))
            {
                capturedPiece = KING;
            }
            switch (capturedPiece)
            {
            case PAWN:
                capturedPieceValue = PAWN_VALUE;
                break;
            case KNIGHT:
                capturedPieceValue = KNIGHT_VALUE;
                break;
            case BISHOP:
                capturedPieceValue = BISHOP_VALUE;
                break;
            case ROOK:
                capturedPieceValue = ROOK_VALUE;
                break;
            case QUEEN:
                capturedPieceValue = QUEEN_VALUE;
                break;
            case KING:
                capturedPieceValue = KING_VALUE;
                break;
            default:
                capturedPieceValue = 0;
                break;
            }

            int attackerPieceValue = 0;
            PieceType attackerPiece = NO_PIECE;
            if (bitboardIsBitSet(board->whitePawns, FROM_SQ(currMove)) || bitboardIsBitSet(board->blackPawns, FROM_SQ(currMove)))
            {
                attackerPiece = PAWN;
            }
            else if (bitboardIsBitSet(board->whiteKnights, FROM_SQ(currMove)) || bitboardIsBitSet(board->blackKnights, FROM_SQ(currMove)))
            {
                attackerPiece = KNIGHT;
            }
            else if (bitboardIsBitSet(board->whiteBishops, FROM_SQ(currMove)) || bitboardIsBitSet(board->blackBishops, FROM_SQ(currMove)))
            {
                attackerPiece = BISHOP;
            }
            else if (bitboardIsBitSet(board->whiteRooks, FROM_SQ(currMove)) || bitboardIsBitSet(board->blackRooks, FROM_SQ(currMove)))
            {
                attackerPiece = ROOK;
            }
            else if (bitboardIsBitSet(board->whiteQueens, FROM_SQ(currMove)) || bitboardIsBitSet(board->blackQueens, FROM_SQ(currMove)))
            {
                attackerPiece = QUEEN;
            }
            else if (bitboardIsBitSet(board->whiteKing, FROM_SQ(currMove)) || bitboardIsBitSet(board->blackKing, FROM_SQ(currMove)))
            {
                attackerPiece = KING;
            }

            switch (attackerPiece)
            {
            case PAWN:
                attackerPieceValue = PAWN_VALUE;
                break;
            case KNIGHT:
                attackerPieceValue = KNIGHT_VALUE;
                break;
            case BISHOP:
                attackerPieceValue = BISHOP_VALUE;
                break;
            case ROOK:
                attackerPieceValue = ROOK_VALUE;
                break;
            case QUEEN:
                attackerPieceValue = QUEEN_VALUE;
                break;
            case KING:
                attackerPieceValue = KING_VALUE;
                break;
            default:
                attackerPieceValue = 0;
                break;
            }

            scores[i] = 1000000 + capturedPieceValue - attackerPieceValue;
        }
        else
        {
            scores[i] = 0; // Non-captures get a base score of 0
        }
    }
}

// TODO: implement iterative deepening search + time management
int negamax(CBoard *node, int depth, int alpha, int beta, Color color)
{
    // check if depth is 0 or game over
    // TODO: implement quiescence search
    if (depth == 0)
    {

        return evaluateBoard(node);
    }

    MoveList moveList = generateLegalMoves(node);
    int scores[moveList.count];
    scoreMoves(node, &moveList, scores);

    // check for checkmate or stalemate
    if (isKingInCheck(node, color) && moveList.count == 0)
    {
        return -200000000 + depth; // Large negative value for checkmate
    }
    if (!isKingInCheck(node, color) && moveList.count == 0)
    {
        return 0; // Draw score for stalemate
    }

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

int iterativeDeepeningSearchEval(CBoard *board, int maxDepth)
{
    int bestEval = -200000000;
    for (int depth = 1; depth <= maxDepth; depth++)
    {
        int eval = negamax(board, depth, -200000000, 200000000, board->sideToMove);
        if (eval > bestEval)
        {
            bestEval = eval;
        }
    }
    return bestEval;
}