#include "search/search.h"
#include "eval/hceval.h"
#include "board/cboard.h"
#include "eval/hceval.h"
#include "movegen/movegen.h"
#include "movegen/move_make.h"
#include "search/tt.h"
#include "board/zobrist.h"
#include "engine/engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>
Move killerMoves[MAX_PLY][MAX_KILLER_MOVES]; // [depth][idx] where 0 is newest killer, 1 is previous killer
int historyHeuristic[2][64][64];             // [color][from][to]

static int pieceValue(PieceType piece);
static long long computeTimeBudgetMs(const CBoard *board, SearchLimits limits);

static const int TT_MOVE_SCORE = 2000000;
static const int GOOD_CAPTURE_BASE_SCORE = 1200000;
static const int KILLER_1_SCORE = 900000;
static const int KILLER_2_SCORE = 800000;
static const int BAD_CAPTURE_BASE_SCORE = 100000;
static const int HISTORY_MAX = 200000;
static const int MATE_SCORE = 200000000;
static const int MATE_THRESHOLD = 199999000;

static PieceType capturedPieceForMove(const CBoard *board, Move move)
{
    if (move_is_enpassant(move))
    {
        return PAWN;
    }

    Square to = getToSquare(move);
    if (to == NO_SQUARE)
    {
        return NO_PIECE;
    }
    return getPieceAtSquare(board, to);
}

static bool isPromotionCapture(const CBoard *board, Move move)
{
    if (!move_is_promotion(move))
    {
        return false;
    }

    Square to = getToSquare(move);
    if (to == NO_SQUARE)
    {
        return false;
    }

    if (board->sideToMove == WHITE)
    {
        return bitboardCheckSquareBit(board->blackPieces, to);
    }
    return bitboardCheckSquareBit(board->whitePieces, to);
}

static bool isCaptureLike(CBoard *board, Move move)
{
    return move_is_capture(board, move) || move_is_enpassant(move) || isPromotionCapture(board, move);
}

static bool isQuietMove(CBoard *board, Move move)
{
    return !move_is_capture(board, move) && !move_is_enpassant(move) && !move_is_promotion(move);
}

static bool isMateScore(int score)
{
    return score >= MATE_THRESHOLD || score <= -MATE_THRESHOLD;
}

static int toTTScore(int score, int ply)
{
    if (score >= MATE_THRESHOLD)
    {
        return score + ply;
    }
    if (score <= -MATE_THRESHOLD)
    {
        return score - ply;
    }
    return score;
}

static int fromTTScore(int score, int ply)
{
    if (score >= MATE_THRESHOLD)
    {
        return score - ply;
    }
    if (score <= -MATE_THRESHOLD)
    {
        return score + ply;
    }
    return score;
}

static void updateHistory(Color side, Move move, int depth)
{
    if (side != WHITE && side != BLACK)
    {
        return;
    }

    Square from = getFromSquare(move);
    Square to = getToSquare(move);
    if (from == NO_SQUARE || to == NO_SQUARE)
    {
        return;
    }

    int bonus = depth * depth;
    int *entry = &historyHeuristic[(int)side][(int)from][(int)to];
    long long updated = (long long)(*entry) + bonus;
    if (updated > HISTORY_MAX)
    {
        updated = HISTORY_MAX;
    }
    else if (updated < -HISTORY_MAX)
    {
        updated = -HISTORY_MAX;
    }
    *entry = (int)updated;
}

static void penalizeHistory(Color side, Move move, int depth)
{
    if (side != WHITE && side != BLACK)
    {
        return;
    }

    Square from = getFromSquare(move);
    Square to = getToSquare(move);
    if (from == NO_SQUARE || to == NO_SQUARE)
    {
        return;
    }

    int malus = depth * depth;
    int *entry = &historyHeuristic[(int)side][(int)from][(int)to];
    long long updated = (long long)(*entry) - malus;
    if (updated > HISTORY_MAX)
    {
        updated = HISTORY_MAX;
    }
    else if (updated < -HISTORY_MAX)
    {
        updated = -HISTORY_MAX;
    }
    *entry = (int)updated;
}

static void ageHistory(void)
{
    for (int side = 0; side < 2; side++)
    {
        for (int from = 0; from < 64; from++)
        {
            for (int to = 0; to < 64; to++)
            {
                historyHeuristic[side][from][to] /= 2;
            }
        }
    }
}

void clearSearchHeuristics()
{
    for (int ply = 0; ply < MAX_PLY; ply++)
    {
        killerMoves[ply][0] = MOVE_NONE;
        killerMoves[ply][1] = MOVE_NONE;
    }
    memset(historyHeuristic, 0, sizeof(historyHeuristic));
}

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

static SearchLimits activeSearchLimits;
static atomic_llong searchedNodes = 0;
static long long searchStartMs = 0;
static atomic_llong searchDeadlineMs = -1;
static atomic_int activeSearchSideToMove = WHITE;

static long long nowMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + (long long)ts.tv_nsec / 1000000LL;
}

static bool moveAllowedBySearchMoves(Move move, const MoveList *searchMoves)
{
    if (searchMoves->count <= 0)
    {
        return true;
    }

    for (int i = 0; i < searchMoves->count; i++)
    {
        if (move == searchMoves->moves[i])
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

    long long deadline = atomic_load(&searchDeadlineMs);
    if (!atomic_load(&engine_is_pondering) && deadline >= 0 && nowMs() >= deadline)
    {
        atomic_store(&engine_stop_search, true);
        return true;
    }

    return false;
}

void onPonderHit(void)
{
    atomic_store(&engine_is_pondering, false);

    Color side = (Color)atomic_load(&activeSearchSideToMove);
    if (side != WHITE && side != BLACK)
    {
        atomic_store(&searchDeadlineMs, -1);
        return;
    }

    CBoard budgetBoard = {0};
    budgetBoard.sideToMove = side;

    long long budgetMs = computeTimeBudgetMs(&budgetBoard, activeSearchLimits);
    long long deadlineMs = (budgetMs > 0) ? (nowMs() + budgetMs) : -1;
    atomic_store(&searchDeadlineMs, deadlineMs);
}

static long long computeTimeBudgetMs(const CBoard *board, SearchLimits limits)
{
    if (limits.searchMoveTimeLimitMs > 0)
    {
        return limits.searchMoveTimeLimitMs;
    }

    if (limits.infiniteSearch)
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

typedef struct
{
    uint8_t previousEpSquare;
    uint16_t previousHalfmoveClock;
    uint16_t previousFullmoveNumber;
    Color previousSideToMove;
    uint64_t previousZobristKey;
} NullMoveUndo;

static NullMoveUndo makeNullMove(CBoard *board)
{
    NullMoveUndo undo;
    undo.previousEpSquare = board->epSquare;
    undo.previousHalfmoveClock = board->halfmoveClock;
    undo.previousFullmoveNumber = board->fullmoveNumber;
    undo.previousSideToMove = board->sideToMove;
    undo.previousZobristKey = board->zobristKey;

    zobristToggleEnPassant(&board->zobristKey, board, board->epSquare);
    board->epSquare = NO_SQUARE;
    board->halfmoveClock++;

    board->sideToMove = (board->sideToMove == WHITE) ? BLACK : WHITE;
    if (board->sideToMove == WHITE)
    {
        board->fullmoveNumber++;
    }

    zobristToggleSide(&board->zobristKey);

    return undo;
}

static void unmakeNullMove(CBoard *board, NullMoveUndo undo)
{
    board->epSquare = undo.previousEpSquare;
    board->halfmoveClock = undo.previousHalfmoveClock;
    board->fullmoveNumber = undo.previousFullmoveNumber;
    board->sideToMove = undo.previousSideToMove;
    board->zobristKey = undo.previousZobristKey;
}

static void moveToUciString(Move move, char *out)
{
    if (getFromSquare(move) == NO_SQUARE || getToSquare(move) == NO_SQUARE)
    {
        strcpy(out, "0000");
        return;
    }

    out[0] = (char)('a' + (getFromSquare(move) % 8));
    out[1] = (char)('1' + (getFromSquare(move) / 8));
    out[2] = (char)('a' + (getToSquare(move) % 8));
    out[3] = (char)('1' + (getToSquare(move) / 8));

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

static int searchRootBestMove(CBoard *board, int depth, Move *prevBestMove)
{
    MoveList moveList;
    initMoveList(&moveList);
    generateLegalMoves(board, &moveList);

    if (moveList.count == 0)
    {
        *prevBestMove = createMove(NO_SQUARE, NO_SQUARE, 0, 0);
        return isKingInCheck(board, board->sideToMove) ? -MATE_SCORE : 0;
    }

    // check if prev best move is valid / legal in this position, if so prioritize it
    // if not fall back to probing the TT for a move to prioritize
    Move ttMove = MOVE_NONE;
    bool foundPrev = false;
    if (getFromSquare(*prevBestMove) != NO_SQUARE)
    {
        for (int i = 0; i < moveList.count; i++)
        {
            if (moveList.moves[i] == *prevBestMove)
            {
                foundPrev = true;
                break;
            }
        }
        if (foundPrev)
        {
            ttMove = *prevBestMove;
        }
    }

    if (!foundPrev)
    {
        TTEntry *ttEntry = probeTT(board->zobristKey);
        if (ttEntry && ttEntry->zobristKey == board->zobristKey)
        {
            ttMove = ttEntry->bestMove;
        }
    }

    ScoredMove scoredMoves[256];
    scoreMoves(board, &moveList, scoredMoves, ttMove, 0);

    int alpha = -200000000;
    int beta = 200000000;
    int bestScore = -MATE_SCORE;
    Move bestMove = createMove(NO_SQUARE, NO_SQUARE, 0, 0);
    bool searchedAtLeastOneMove = false;
    for (int i = 0; i < moveList.count; i++)
    {
        if (shouldStopSearch())
        {
            break;
        }
        pickNextBestMove(scoredMoves, i, moveList.count);

        Move move = scoredMoves[i].move;

        char currMoveUci[6];
        moveToUciString(move, currMoveUci);
        // printf("info depth %d currmove %s currmovenumber %d\n", depth, currMoveUci, i + 1);

        // Skip moves that aren't in the searchMoves list (if searchMoves is non-empty)
        if (!moveAllowedBySearchMoves(move, &activeSearchLimits.searchMoves))
        {
            continue;
        }

        UndoInfo undoInfo = makeMove(board, move);
        int eval = -negamax(board, depth - 1, -beta, -alpha, board->sideToMove, 1);
        unmakeMove(board, move, undoInfo);
        searchedAtLeastOneMove = true;

        if (shouldStopSearch())
        {
            break;
        }

        if (eval > bestScore || getFromSquare(bestMove) == NO_SQUARE)
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
        *prevBestMove = createMove(NO_SQUARE, NO_SQUARE, 0, 0);
        return -MATE_SCORE;
    }
    storeTT(board->zobristKey, depth, toTTScore(bestScore, 0), TT_PV, bestMove);
    *prevBestMove = bestMove;
    return bestScore;
}

// Define the global atomic flag here
atomic_bool engine_stop_search = false;
atomic_bool engine_is_pondering = false;

// The entry point for the search thread
void *search_worker(void *arg)
{
    // Cast and extract the data
    SearchThreadData *data = (SearchThreadData *)arg;
    CBoard searchBoard = data->board;
    SearchLimits limits = data->limits;
    // clear all killer moves
    clearSearchHeuristics();
    activeSearchLimits = limits;
    atomic_store(&activeSearchSideToMove, (int)searchBoard.sideToMove);
    atomic_store(&searchedNodes, 0);
    searchStartMs = nowMs();
    long long deadlineMs = -1;
    if (!limits.ponder)
    {
        long long budgetMs = computeTimeBudgetMs(&searchBoard, limits);
        deadlineMs = (budgetMs > 0) ? (searchStartMs + budgetMs) : -1;
    }
    atomic_store(&searchDeadlineMs, deadlineMs);

    // We copied the data to local stack variables, so free the allocated payload
    free(data);

    int currentDepth = 1;
    int bestScore = 0;
    Move bestMove = createMove(NO_SQUARE, NO_SQUARE, 0, 0);

    int maxDepth;
    if (limits.searchDepthLimit > 0)
    {
        maxDepth = limits.searchDepthLimit;
    }
    else if (limits.infiniteSearch || limits.ponder)
    {
        maxDepth = INT_MAX;
    }
    else
    {
        maxDepth = 100;
    }

    // The Iterative Deepening Loop
    while (currentDepth <= maxDepth)
    {
        if (shouldStopSearch())
        {
            break;
        }

        Move depthBestMove = bestMove;
        int score = searchRootBestMove(&searchBoard, currentDepth, &depthBestMove);
        if (!shouldStopSearch() && getFromSquare(depthBestMove) != NO_SQUARE)
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
        if (abs(bestScore) > 100000000)
        {
            int mateMoves = (MATE_SCORE - abs(bestScore)) / 2;
            int mateScore = bestScore >= 0 ? mateMoves : -mateMoves;
            printf("info depth %d score mate %d nodes %lld time %lld nps %lld pv %s\n",
                   currentDepth, mateScore, nodes, elapsed, nps, pvString);
        }
        else
        {
            printf("info depth %d score cp %d nodes %lld time %lld nps %lld pv %s\n",
                   currentDepth, bestScore, nodes, elapsed, nps, pvString);
        }

        ageHistory();

        currentDepth++;

        if (limits.searchMoveTimeLimitMs > 0 && shouldStopSearch())
        {
            break;
        }

        if (!atomic_load(&engine_is_pondering) && limits.searchForMateInNMoves > 0 && abs(bestScore) > 100000000)
        {
            int mateMoves = (MATE_SCORE - abs(bestScore)) / 2;
            if (mateMoves <= limits.searchForMateInNMoves)
            {
                break;
            }
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
    atomic_store(&engine_is_pondering, goCmd.ponder);
    state->isSearching = true;
    // 3. Allocate the thread payload. (Using malloc so it safely survives the thread launch)
    SearchThreadData *threadData = malloc(sizeof(SearchThreadData));
    if (threadData == NULL)
    {
        printf("info string memory allocation failed\n");
        return;
    }

    // snapshot of the board state. This prevents race conditions if the GUI
    // suddenly sends another "position" command while pondering.
    threadData->board = state->board;
    threadData->limits = goCmd;

    if (pthread_create(&state->searchThread, NULL, search_worker, threadData) != 0)
    {
        printf("info string failed to create search thread\n");
        free(threadData);
        state->isSearching = false;
    }
}

void scoreMoves(CBoard *board, MoveList *moveList, ScoredMove *scoredMoves, Move ttMove, int ply)
{
    Color side = board->sideToMove;

    for (int i = 0; i < moveList->count; i++)
    {
        Move currMove = moveList->moves[i];
        int score = 0;

        scoredMoves[i].move = currMove;
        if (ttMove != MOVE_NONE && currMove == ttMove)
        {
            score = TT_MOVE_SCORE;
        }
        else if (isCaptureLike(board, currMove) || move_is_promotion(currMove))
        {
            PieceType attackerPiece = getPieceAtSquare(board, getFromSquare(currMove));
            PieceType capturedPiece = capturedPieceForMove(board, currMove);
            int mvvLva = 0;
            if (capturedPiece != NO_PIECE && attackerPiece != NO_PIECE)
            {
                mvvLva = pieceValue(capturedPiece) - pieceValue(attackerPiece);
            }

            int promoBonus = 0;
            if (move_is_promotion(currMove))
            {
                promoBonus = pieceValue(getPromotionPieceType(currMove));
            }

            if (mvvLva >= 0 || move_is_promotion(currMove))
            {
                score = GOOD_CAPTURE_BASE_SCORE + mvvLva + promoBonus;
            }
            else
            {
                score = BAD_CAPTURE_BASE_SCORE + mvvLva + promoBonus;
            }
        }
        else if (ply < MAX_PLY)
        {
            if (currMove == killerMoves[ply][0])
            {
                score = KILLER_1_SCORE;
            }
            else if (currMove == killerMoves[ply][1])
            {
                score = KILLER_2_SCORE;
            }
            else
            {
                Square from = getFromSquare(currMove);
                Square to = getToSquare(currMove);
                if (from != NO_SQUARE && to != NO_SQUARE)
                {
                    score = historyHeuristic[(int)side][(int)from][(int)to];
                }
            }
        }
        else
        {
            Square from = getFromSquare(currMove);
            Square to = getToSquare(currMove);
            if (from != NO_SQUARE && to != NO_SQUARE)
            {
                score = historyHeuristic[(int)side][(int)from][(int)to];
            }
        }
        scoredMoves[i].score = score;
    }
}

void pickNextBestMove(ScoredMove *scoredMoves, int start, int count)
{
    for (int i = start; i < count; i++)
    {
        if (scoredMoves[i].score > scoredMoves[start].score)
        {
            // Swap
            ScoredMove temp = scoredMoves[start];
            scoredMoves[start] = scoredMoves[i];
            scoredMoves[i] = temp;
        }
    }
}

int quiescence(CBoard *node, int alpha, int beta, int ply)
{
    atomic_fetch_add(&searchedNodes, 1);

    if (shouldStopSearch())
    {
        return evaluateBoard(node);
    }

    if (ply >= MAX_PLY - 1)
    {
        return evaluateBoard(node);
    }

    bool inCheck = isKingInCheck(node, node->sideToMove);
    if (!inCheck)
    {
        int standPat = evaluateBoard(node);
        if (standPat >= beta)
        {
            return standPat;
        }
        if (standPat > alpha)
        {
            alpha = standPat;
        }
    }

    MoveList moveList;
    initMoveList(&moveList);
    if (inCheck)
    {
        generateLegalMoves(node, &moveList);
    }
    else
    {
        generateCaptureMoves(node, &moveList);
    }

    if (moveList.count == 0)
    {
        if (inCheck)
        {
            return -MATE_SCORE + ply;
        }
        return evaluateBoard(node);
    }

    TTEntry *ttEntry = probeTT(node->zobristKey);
    Move ttBestMove = MOVE_NONE;
    if (ttEntry && ttEntry->zobristKey == node->zobristKey)
    {
        ttBestMove = ttEntry->bestMove;
    }

    ScoredMove scoredMoves[256];
    scoreMoves(node, &moveList, scoredMoves, ttBestMove, ply);

    for (int i = 0; i < moveList.count; i++)
    {
        if (i % 32 == 0 && shouldStopSearch())
        {
            break;
        }

        pickNextBestMove(scoredMoves, i, moveList.count);
        Move move = scoredMoves[i].move;

        if (!inCheck && !isCaptureLike(node, move) && !move_is_promotion(move))
        {
            continue;
        }

        UndoInfo undoInfo = makeMove(node, move);
        int eval = -quiescence(node, -beta, -alpha, ply + 1);
        unmakeMove(node, move, undoInfo);

        if (eval >= beta)
        {
            return eval;
        }
        if (eval > alpha)
        {
            alpha = eval;
        }
    }

    return alpha;
}

int negamax(CBoard *node, int depth, int alpha, int beta, Color color, int ply)
{
    atomic_fetch_add(&searchedNodes, 1);

    if (shouldStopSearch())
    {
        return evaluateBoard(node);
    }

    if (ply >= MAX_PLY - 1)
    {
        return quiescence(node, alpha, beta, ply);
    }

    Color side = color;
    int originalAlpha = alpha;
    bool inCheck = isKingInCheck(node, side);

    TTEntry *ttEntry = probeTT(node->zobristKey);
    Move ttBestMove = MOVE_NONE;
    if (ttEntry && ttEntry->zobristKey == node->zobristKey)
    {
        ttBestMove = ttEntry->bestMove;
        int ttScore = fromTTScore(ttEntry->score, ply);
        if (ttEntry->depth >= depth)
        {
            if (ttEntry->bound == TT_PV)
            {
                return ttScore;
            }
            else if (ttEntry->bound == TT_CUT && ttScore >= beta)
            {
                return ttScore;
            }
            else if (ttEntry->bound == TT_ALL && ttScore <= alpha)
            {
                return ttScore;
            }
            if (alpha >= beta)
            {
                return ttScore;
            }
        }
    }

    if (depth == 0)
    {
        return quiescence(node, alpha, beta, ply);
    }

    // Null-move pruning (skip in check or near-mate windows)
    if (!inCheck && depth >= 3 && !isMateScore(alpha) && !isMateScore(beta))
    {
        int reduction = 2 + (depth >= 6 ? 1 : 0);
        NullMoveUndo nullUndo = makeNullMove(node);
        Color nextSide = (side == WHITE) ? BLACK : WHITE;
        int nullScore = -negamax(node, depth - 1 - reduction, -beta, -beta + 1, nextSide, ply + 1);
        unmakeNullMove(node, nullUndo);

        if (nullScore >= beta)
        {
            return nullScore;
        }
    }

    MoveList moveList;
    initMoveList(&moveList);
    genAllPseudoLegalMoves(node, &moveList);
    ScoredMove scoredMoves[256];
    scoreMoves(node, &moveList, scoredMoves, ttBestMove, ply);

    Move bestMoveAtNode = createMove(NO_SQUARE, NO_SQUARE, 0, 0);
    int maxEval = -MATE_SCORE;
    int legalMovesSearched = 0;

    for (int i = 0; i < moveList.count; i++)
    {
        if (i % 32 == 0 && shouldStopSearch())
        {
            break;
        }
        pickNextBestMove(scoredMoves, i, moveList.count);
        Move move = scoredMoves[i].move;

        bool quiet = isQuietMove(node, move);

        // Make the move
        UndoInfo undoInfo = makeMove(node, move);

        if (isKingInCheck(node, side))
        {
            unmakeMove(node, move, undoInfo);
            continue;
        }

        legalMovesSearched++;
        Color nextSide = (side == WHITE) ? BLACK : WHITE;
        int eval;

        if (legalMovesSearched == 1)
        {
            eval = -negamax(node, depth - 1, -beta, -alpha, nextSide, ply + 1);
        }
        else
        {
            int reducedDepth = depth - 1;
            bool canReduce = depth >= 3 && legalMovesSearched >= 4 && !inCheck && quiet;
            if (canReduce)
            {
                reducedDepth = depth - 2;
            }

            // PVS scout search
            eval = -negamax(node, reducedDepth, -alpha - 1, -alpha, nextSide, ply + 1);

            // Re-search if scout failed high
            if (eval > alpha)
            {
                eval = -negamax(node, depth - 1, -beta, -alpha, nextSide, ply + 1);
            }
        }

        // Unmake the move
        unmakeMove(node, move, undoInfo);

        int alphaBefore = alpha;

        if (eval > maxEval)
        {
            maxEval = eval;
            bestMoveAtNode = move;
        }
        if (maxEval > alpha)
        {
            alpha = maxEval;
        }
        else if (quiet && eval <= alphaBefore)
        {
            penalizeHistory(side, move, depth);
        }

        if (alpha >= beta)
        {
            // Store killer move
            if (ply < MAX_PLY && quiet)
            {
                if (killerMoves[ply][0] != move)
                {
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = move;
                }
                updateHistory(side, move, depth);
            }
            break; // beta cutoff
        }
    }

    if (legalMovesSearched == 0)
    {
        if (inCheck)
        {
            return -MATE_SCORE + ply;
        }
        return 0;
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
    storeTT(node->zobristKey, depth, toTTScore(maxEval, ply), bound, bestMoveAtNode);
    return maxEval;
}
