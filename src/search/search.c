#include "search/search.h"
#include "hcevaluation/hceval.h"
#include "board/cboard.h"
#include "hcevaluation/hceval.h"
#include "movegen/movegen.h"
#include "movegen/move_make.h"
#include "search/tt.h"
#include "engine/engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

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

static SearchLimits activeSearchLimits;
static atomic_llong searchedNodes = 0;
static long long searchStartMs = 0;
static long long searchDeadlineMs = -1;

static long long nowMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + (long long)ts.tv_nsec / 1000000LL;
}

static bool movesEqual(Move a, Move b)
{
    return a.from == b.from && a.to == b.to && a.flag == b.flag;
}

static bool moveAllowedBySearchMoves(Move move, const MoveList *searchMoves)
{
    if (searchMoves->count <= 0)
    {
        return true;
    }

    for (int i = 0; i < searchMoves->count; i++)
    {
        if (movesEqual(move, searchMoves->moves[i]))
        {
            return true;
        }
    }

    return false;
}

static bool shouldStopSearch(void)
{
    if (atomic_load(&engine_stop_search))
    {
        return true;
    }

    if (activeSearchLimits.searchNodeLimit > 0 && atomic_load(&searchedNodes) >= activeSearchLimits.searchNodeLimit)
    {
        atomic_store(&engine_stop_search, true);
        return true;
    }

    if (searchDeadlineMs >= 0 && nowMs() >= searchDeadlineMs)
    {
        atomic_store(&engine_stop_search, true);
        return true;
    }

    return false;
}

static long long computeTimeBudgetMs(const CBoard *board, SearchLimits limits)
{
    if (limits.searchMoveTimeLimitMs > 0)
    {
        return limits.searchMoveTimeLimitMs;
    }

    if (limits.infiniteSearch || limits.ponder)
    {
        return -1;
    }

    int remaining = (board->sideToMove == WHITE) ? limits.timeForWhiteMs : limits.timeForBlackMs;
    int increment = (board->sideToMove == WHITE) ? limits.incrementForWhiteMs : limits.incrementForBlackMs;
    int movesToGo = limits.movesUntilNextTimeControl > 0 ? limits.movesUntilNextTimeControl : 30;

    if (remaining <= 0)
    {
        return -1;
    }

    long long budget = (long long)remaining / movesToGo + (long long)increment / 2;
    if (budget < 10)
    {
        budget = 10;
    }

    long long maxSpend = (long long)remaining - 5;
    if (maxSpend > 0 && budget > maxSpend)
    {
        budget = maxSpend;
    }

    return budget;
}

static void moveToUciString(Move move, char *out)
{
    if (move.from == NO_SQUARE || move.to == NO_SQUARE)
    {
        strcpy(out, "0000");
        return;
    }

    out[0] = (char)('a' + (move.from % 8));
    out[1] = (char)('1' + (move.from / 8));
    out[2] = (char)('a' + (move.to % 8));
    out[3] = (char)('1' + (move.to / 8));

    if (move_is_promotion(move))
    {
        PieceType promo = getPromotionPieceType(move);
        char promoChar = 'q';
        if (promo == KNIGHT)
            promoChar = 'n';
        else if (promo == BISHOP)
            promoChar = 'b';
        else if (promo == ROOK)
            promoChar = 'r';
        else
            promoChar = 'q';

        out[4] = promoChar;
        out[5] = '\0';
        return;
    }

    out[4] = '\0';
}

static int searchRootBestMove(CBoard *board, int depth, Move *outBestMove)
{
    MoveList moveList;
    initMoveList(&moveList);
    generateLegalMoves(board, &moveList);

    if (moveList.count == 0)
    {
        *outBestMove = (Move){.from = NO_SQUARE, .to = NO_SQUARE, .flag = 0};
        return isKingInCheck(board, board->sideToMove) ? -200000000 : 0;
    }

    ScoredMove scoredMoves[256];
    scoreMoves(board, &moveList, scoredMoves, (Move){.from = NO_SQUARE, .to = NO_SQUARE, .flag = 0});
    sortScoredMoves(scoredMoves, moveList.count);

    int alpha = -200000000;
    int beta = 200000000;
    int bestScore = -200000000;
    Move bestMove = (Move){.from = NO_SQUARE, .to = NO_SQUARE, .flag = 0};
    bool searchedAtLeastOneMove = false;

    for (int i = 0; i < moveList.count; i++)
    {
        if (shouldStopSearch())
        {
            break;
        }

        Move move = scoredMoves[i].move;
        if (!moveAllowedBySearchMoves(move, &activeSearchLimits.searchMoves))
        {
            continue;
        }

        UndoInfo undoInfo = makeMove(board, move);
        int eval = -negamax(board, depth - 1, -beta, -alpha, board->sideToMove);
        unmakeMove(board, move, undoInfo);
        searchedAtLeastOneMove = true;

        if (shouldStopSearch())
        {
            break;
        }

        if (eval > bestScore || bestMove.from == NO_SQUARE)
        {
            bestScore = eval;
            bestMove = move;
        }

        if (eval > alpha)
        {
            alpha = eval;
        }
    }

    if (!searchedAtLeastOneMove)
    {
        *outBestMove = (Move){.from = NO_SQUARE, .to = NO_SQUARE, .flag = 0};
        return -200000000;
    }

    *outBestMove = bestMove;
    return bestScore;
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

    activeSearchLimits = limits;
    atomic_store(&searchedNodes, 0);
    searchStartMs = nowMs();
    long long budgetMs = computeTimeBudgetMs(&searchBoard, limits);
    searchDeadlineMs = (budgetMs > 0) ? (searchStartMs + budgetMs) : -1;

    // We copied the data to local stack variables, so free the allocated payload
    free(data);

    int currentDepth = 1;
    int bestScore = 0;
    Move bestMove = {.from = NO_SQUARE, .to = NO_SQUARE, .flag = 0};

    int maxDepth = limits.searchDepthLimit > 0 ? limits.searchDepthLimit : 100;

    // 2. The Iterative Deepening Loop
    while (currentDepth <= maxDepth)
    {

        // 3. Check the stop flag periodically!
        if (shouldStopSearch())
        {
            break; // Break out of iterative deepening immediately
        }

        Move depthBestMove = bestMove;
        int score = searchRootBestMove(&searchBoard, currentDepth, &depthBestMove);
        if (!shouldStopSearch() && depthBestMove.from != NO_SQUARE)
        {
            bestMove = depthBestMove;
            bestScore = score;
        }

        long long elapsed = nowMs() - searchStartMs;
        long long nodes = atomic_load(&searchedNodes);
        long long nps = elapsed > 0 ? (nodes * 1000LL) / elapsed : nodes;
        Move pvLine[256];
        int pvLength = extractPVLine(&searchBoard, pvLine, currentDepth);
        char pvString[2048] = "";
        for (int i = 0; i < pvLength; i++)
        {
            char moveStr[6];
            moveToUciString(pvLine[i], moveStr);
            strcat(pvString, moveStr);
            strcat(pvString, " ");
        }
        printf("info depth %d score cp %d nodes %lld time %lld nps %lld pv %s\n",
               currentDepth, bestScore, nodes, elapsed, nps, pvString);

        currentDepth++;

        if (limits.searchMoveTimeLimitMs > 0 && shouldStopSearch())
        {
            break;
        }
    }

    char bestMoveUci[6];
    moveToUciString(bestMove, bestMoveUci);
    printf("bestmove %s\n", bestMoveUci);
    fflush(stdout);

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

void scoreMoves(CBoard *board, MoveList *moveList, ScoredMove *scoredMoves, Move ttMove)
{
    for (int i = 0; i < moveList->count; i++)
    {
        Move currMove = moveList->moves[i];
        int score = 0;

        scoredMoves[i].move = currMove;
        if (ttMove.from != NO_SQUARE && movesEqual(currMove, ttMove))
        {
            score = 2000000; // TT move gets highest priority
        }
        else
        {
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
    atomic_fetch_add(&searchedNodes, 1);

    if (shouldStopSearch())
    {
        return evaluateBoard(node);
    }

    int originalAlpha = alpha;
    TTEntry *ttEntry = probeTT(node->zobristKey);
    Move ttBestMove = (Move){.from = NO_SQUARE, .to = NO_SQUARE, .flag = 0};
    if (ttEntry && ttEntry->zobristKey == node->zobristKey)
    {
        ttBestMove = ttEntry->bestMove;
        if (ttEntry->depth >= depth)
        {
            if (ttEntry->bound == TT_PV)
            {
                return ttEntry->score;
            }
            else if (ttEntry->bound == TT_CUT && ttEntry->score >= beta)
            {
                return ttEntry->score;
            }
            else if (ttEntry->bound == TT_ALL && ttEntry->score <= alpha)
            {
                return ttEntry->score;
            }
            if (alpha >= beta)
            {
                return ttEntry->score;
            }
        }
    }
    if (depth == 0)
    {
        return evaluateBoard(node);
    }

    MoveList moveList;
    initMoveList(&moveList);
    generateLegalMoves(node, &moveList);
    ScoredMove scoredMoves[256];
    scoreMoves(node, &moveList, scoredMoves, ttBestMove);
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
        if (i % 32 == 0 && shouldStopSearch())
        {
            break;
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
    TTBound bound = TT_PV;
    if (maxEval <= originalAlpha)
    {
        bound = TT_ALL;
    }
    else if (maxEval >= beta)
    {
        bound = TT_CUT;
    }
    storeTT(node->zobristKey, depth, maxEval, bound, ttBestMove);
    return maxEval;
}
