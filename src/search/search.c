#include "search/search.h"
#include "hcevaluation/hceval.h"
#include "board/cboard.h"
#include "hcevaluation/hceval.h"
#include "movegen/movegen.h"
#include "movegen/move_make.h"
#include "engine/engine.h"
#include <stdio.h>
#include <stdlib.h>

static int pieceValue(PieceType piece)
{
    switch (piece)
    {
    case PAWN:
        return PAWN_VALUE;
    case KNIGHT:
        return KNIGHT_VALUE;
    case BISHOP:
        return BISHOP_VALUE;
    case ROOK:
        return ROOK_VALUE;
    case QUEEN:
        return QUEEN_VALUE;
    case KING:
        return KING_VALUE;
    default:
        return 0;
    }
}

static PieceType getPieceAtSquare(const CBoard *board, Square square)
{
    if (bitboardIsBitSet(board->whitePawns, square) || bitboardIsBitSet(board->blackPawns, square))
    {
        return PAWN;
    }
    if (bitboardIsBitSet(board->whiteKnights, square) || bitboardIsBitSet(board->blackKnights, square))
    {
        return KNIGHT;
    }
    if (bitboardIsBitSet(board->whiteBishops, square) || bitboardIsBitSet(board->blackBishops, square))
    {
        return BISHOP;
    }
    if (bitboardIsBitSet(board->whiteRooks, square) || bitboardIsBitSet(board->blackRooks, square))
    {
        return ROOK;
    }
    if (bitboardIsBitSet(board->whiteQueens, square) || bitboardIsBitSet(board->blackQueens, square))
    {
        return QUEEN;
    }
    if (bitboardIsBitSet(board->whiteKing, square) || bitboardIsBitSet(board->blackKing, square))
    {
        return KING;
    }
    return NO_PIECE;
}

static int compareScoredMovesDescending(const void *a, const void *b)
{
    const ScoredMove *moveA = (const ScoredMove *)a;
    const ScoredMove *moveB = (const ScoredMove *)b;
    return moveB->score - moveA->score;
}

// Define the global atomic flag here
atomic_bool engine_stop_search = false;

// The entry point for the search thread
void *search_worker(void *arg)
{
    // 1. Cast and extract the data
    SearchThreadData *data = (SearchThreadData *)arg;
    CBoard searchBoard = data->board;
    SearchLimits limits = data->limits;
    (void)limits;

    // We copied the data to local stack variables, so free the allocated payload
    free(data);

    int currentDepth = 1;
    Move bestMove = {0}; // Add your null move initializer
    (void)bestMove;

    // 2. The Iterative Deepening Loop
    while (currentDepth <= 100)
    { // Max depth fallback

        // 3. Check the stop flag periodically!
        if (atomic_load(&engine_stop_search))
        {
            break; // Break out of iterative deepening immediately
        }

        // --- Call your Alpha-Beta function here ---
        int score = negamax(&searchBoard, currentDepth, -200000000, 200000000, searchBoard.sideToMove);
        (void)score;

        // Update bestMove if search completed this depth without being stopped

        currentDepth++;
    }

    // 4. Print the final best move back to the GUI
    // Note: Ensure your move formatting logic goes here
    printf("bestmove e2e4\n");

    return NULL;
}

void searchOnGoCommand(UCIState *state, SearchLimits goCmd)
{
    // if search running, stop it before starting a new one
    if (state->isSearching)
    {
        atomic_store(&engine_stop_search, true);
        pthread_join(state->searchThread, NULL);
    }

    // Prepare for the new search
    atomic_store(&engine_stop_search, false);
    state->isSearching = true;
    // 3. Allocate the thread payload. (Using malloc so it safely survives the thread launch)
    SearchThreadData *threadData = malloc(sizeof(SearchThreadData));
    if (threadData == NULL)
    {
        printf("info string memory allocation failed\n");
        return;
    }

    // 4. SNAPSHOT the board state. This prevents race conditions if the GUI
    // suddenly sends another "position" command while pondering.
    threadData->board = state->board;
    threadData->limits = goCmd;

    // 5. Kick off the background thread!
    if (pthread_create(&state->searchThread, NULL, search_worker, threadData) != 0)
    {
        printf("info string failed to create search thread\n");
        free(threadData);
        state->isSearching = false;
    }
}

void scoreMoves(CBoard *board, MoveList *moveList, ScoredMove *scoredMoves)
{
    for (int i = 0; i < moveList->count; i++)
    {
        Move currMove = moveList->moves[i];
        int score = 0;

        scoredMoves[i].move = currMove;

        if (move_is_capture(currMove))
        {
            PieceType capturedPiece = getPieceAtSquare(board, TO_SQ(currMove));
            PieceType attackerPiece = getPieceAtSquare(board, FROM_SQ(currMove));
            score += 1000000 + pieceValue(capturedPiece) - pieceValue(attackerPiece);
        }

        if (move_is_promotion(currMove))
        {
            score += 500000 + pieceValue(getPromotionPieceType(currMove));
        }

        scoredMoves[i].score = score;
    }
}

void sortScoredMoves(ScoredMove *scoredMoves, int count)
{
    qsort(scoredMoves, (size_t)count, sizeof(ScoredMove), compareScoredMovesDescending);
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

    MoveList moveList;
    initMoveList(&moveList);
    generateLegalMoves(node, &moveList);
    ScoredMove scoredMoves[256];
    scoreMoves(node, &moveList, scoredMoves);
    sortScoredMoves(scoredMoves, moveList.count);

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
        if (i % 2048 == 0 && atomic_load(&engine_stop_search))
        {
            break; // Check the stop flag every ~2048 nodes
        }
        Move move = scoredMoves[i].move;

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
