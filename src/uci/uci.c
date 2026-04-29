#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/engine.h"
#include "movegen/move.h"
#include "movegen/move_make.h"
#include "movegen/movegen.h"
#include "search/search.h"
#include "search/tt.h"
#include "uci/uci.h"

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

void skipWhitespace(const char** str);

static void stopSearchIfRunning(UCIState* state)
{
    if (!state->isSearching) {
        return;
    }

    atomic_store(&engine_stop_search, true);
    pthread_join(state->searchThread, NULL);
    state->isSearching = false;
    atomic_store(&engine_is_pondering, false);
}

static bool nextToken(const char** command, char* out, size_t outSize)
{
    skipWhitespace(command);
    if (**command == '\0') {
        return false;
    }

    size_t len = strcspn(*command, " \t\r\n");
    if (len >= outSize) {
        len = outSize - 1;
    }

    strncpy(out, *command, len);
    out[len] = '\0';
    *command += strcspn(*command, " \t\r\n");
    return true;
}

static bool isGoKeyword(const char* token)
{
    return !strcmp(token, "searchmoves") || !strcmp(token, "ponder") || !strcmp(token, "wtime") || !strcmp(token, "btime") || !strcmp(token, "winc") || !strcmp(token, "binc") || !strcmp(token, "movestogo") || !strcmp(token, "depth") || !strcmp(token, "nodes") || !strcmp(token, "mate") || !strcmp(token, "movetime") || !strcmp(token, "infinite");
}

static bool parseNextIntToken(const char** command, int* out)
{
    char token[64];
    if (!nextToken(command, token, sizeof(token))) {
        return false;
    }

    *out = atoi(token);
    return true;
}

static bool equalsIgnoreCase(const char* a, const char* b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static void handleSetOptionCommand(UCIState* state, const char* command)
{
    command += 9; // Skip "setoption"

    char token[64];
    char name[128] = "";
    char value[128] = "";
    bool readingName = false;
    bool readingValue = false;

    while (nextToken(&command, token, sizeof(token))) {
        if (equalsIgnoreCase(token, "name")) {
            readingName = true;
            readingValue = false;
            name[0] = '\0';
            continue;
        }

        if (equalsIgnoreCase(token, "value")) {
            readingValue = true;
            readingName = false;
            value[0] = '\0';
            continue;
        }

        if (readingName) {
            if (name[0] != '\0' && strlen(name) + 1 < sizeof(name)) {
                strncat(name, " ", sizeof(name) - strlen(name) - 1);
            }
            if (strlen(name) + strlen(token) < sizeof(name)) {
                strncat(name, token, sizeof(name) - strlen(name) - 1);
            }
        } else if (readingValue) {
            if (value[0] != '\0' && strlen(value) + 1 < sizeof(value)) {
                strncat(value, " ", sizeof(value) - strlen(value) - 1);
            }
            if (strlen(value) + strlen(token) < sizeof(value)) {
                strncat(value, token, sizeof(value) - strlen(value) - 1);
            }
        }
    }

    if (equalsIgnoreCase(name, "Hash")) {
        stopSearchIfRunning(state);

        if (value[0] == '\0') {
            printf("info string setoption Hash requires a value\n");
            fflush(stdout);
            return;
        }

        char* endptr = NULL;
        errno = 0;
        long mb = strtol(value, &endptr, 10);
        if (errno != 0 || endptr == value || *endptr != '\0') {
            printf("info string Invalid Hash value: %s\n", value);
            fflush(stdout);
            return;
        }

        if (mb < 1) {
            mb = 1;
        } else if (mb > 1024) {
            mb = 1024;
        }

        initTT((size_t)mb);
        printf("info string Hash set to %ld MB\n", mb);
        fflush(stdout);
        return;
    }

    if (equalsIgnoreCase(name, "Clear Hash")) {
        stopSearchIfRunning(state);
        clearTT();
        printf("info string Hash cleared\n");
        fflush(stdout);
        return;
    }

    printf("info string Unsupported option: %s\n", name[0] ? name : "(none)");
    fflush(stdout);
}

int safeLineRead(char* line_input)
{
    if (fgets(line_input, 8192, stdin) == NULL) {
        return 0;
    }
    size_t ender_pos = strcspn(line_input, "\r\n");
    if (ender_pos < strlen(line_input)) {
        line_input[ender_pos] = '\0'; // Remove newline characters
    }
    return 1;
}

void skipWhitespace(const char** str)
{
    while (**str && isspace((unsigned char)**str)) {
        (*str)++;
    }
}

Square algebraicToSquare(const char* squareStr)
{
    if (strlen(squareStr) < 2) {
        return NO_SQUARE;
    }
    char file = squareStr[0];
    char rank = squareStr[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
        return NO_SQUARE;
    }
    int fileIndex = file - 'a';
    int rankIndex = rank - '1';
    return (Square)(rankIndex * 8 + fileIndex);
}

Move parseLongAlgebraicMove(const CBoard* board, const char* moveStr)
{
    if (strlen(moveStr) < 4) {
        printf("info string Invalid move format: %s\n", moveStr);
        return createMove(NO_SQUARE, NO_SQUARE, NORMAL, NO_PIECE);
    }

    Square from = algebraicToSquare(moveStr);
    Square to = algebraicToSquare(moveStr + 2);
    if (from == NO_SQUARE || to == NO_SQUARE) {
        printf("info string Invalid move format: %s\n", moveStr);
        return createMove(NO_SQUARE, NO_SQUARE, NORMAL, NO_PIECE);
    }

    // generateLegalMoves() temporarily makes/unmakes moves internally.
    // Work on a copy here so parsing cannot accidentally perturb live board
    // state.
    CBoard boardCopy = *board;
    MoveList moveList;
    initMoveList(&moveList);
    generateLegalMoves(&boardCopy, &moveList);

    char promotionChar = '\0';
    if (strlen(moveStr) >= 5) {
        promotionChar = (char)tolower((unsigned char)moveStr[4]);
    }

    for (int i = 0; i < moveList.count; i++) {
        if (getFromSquare(moveList.moves[i]) == from && getToSquare(moveList.moves[i]) == to) {
            if (promotionChar == '\0') {
                return moveList.moves[i];
            }

            if (!move_is_promotion(moveList.moves[i])) {
                continue;
            }

            PieceType promoType = getPromotionPieceType(moveList.moves[i]);
            bool promotionMatches = (promotionChar == 'n' && promoType == KNIGHT) || (promotionChar == 'b' && promoType == BISHOP) || (promotionChar == 'r' && promoType == ROOK) || (promotionChar == 'q' && promoType == QUEEN);

            if (promotionMatches) {
                return moveList.moves[i];
            }
            continue;
        }
    }
    printf("info string Move not in legal moves list: %s\n", moveStr);
    return createMove(NO_SQUARE, NO_SQUARE, NORMAL, NO_PIECE);
}

void handleGoCommand(UCIState* state, const char* command)
{
    command += 2; // Skip "go"
    SearchLimits goCmd = { 0 };
    initMoveList(&goCmd.searchMoves);
    bool searchmovesSpecified = false;

    char token[64];
    while (nextToken(&command, token, sizeof(token))) {
        if (!strcmp(token, "searchmoves")) {
            searchmovesSpecified = true;
            while (1) {
                const char* saved = command;
                char moveToken[32];
                if (!nextToken(&command, moveToken, sizeof(moveToken))) {
                    break;
                }

                if (isGoKeyword(moveToken)) {
                    command = saved;
                    break;
                }

                Move move = parseLongAlgebraicMove(&state->board, moveToken);
                if (getFromSquare(move) != NO_SQUARE && getToSquare(move) != NO_SQUARE) {
                    if (goCmd.searchMoves.count < 256) {
                        goCmd.searchMoves.moves[goCmd.searchMoves.count++] = move;
                    }
                } else if (state->debugMode) {
                    printf("info string Invalid move in go searchmoves: %s\n", moveToken);
                }
            }
        } else if (!strcmp(token, "ponder")) {
            goCmd.ponder = true;
        } else if (!strcmp(token, "infinite")) {
            goCmd.infiniteSearch = true;
        } else if (!strcmp(token, "wtime")) {
            parseNextIntToken(&command, &goCmd.timeForWhiteMs);
        } else if (!strcmp(token, "btime")) {
            parseNextIntToken(&command, &goCmd.timeForBlackMs);
        } else if (!strcmp(token, "winc")) {
            parseNextIntToken(&command, &goCmd.incrementForWhiteMs);
        } else if (!strcmp(token, "binc")) {
            parseNextIntToken(&command, &goCmd.incrementForBlackMs);
        } else if (!strcmp(token, "movestogo")) {
            parseNextIntToken(&command, &goCmd.movesUntilNextTimeControl);
        } else if (!strcmp(token, "depth")) {
            parseNextIntToken(&command, &goCmd.searchDepthLimit);
        } else if (!strcmp(token, "nodes")) {
            parseNextIntToken(&command, &goCmd.searchNodeLimit);
        } else if (!strcmp(token, "mate")) {
            parseNextIntToken(&command, &goCmd.searchForMateInNMoves);
        } else if (!strcmp(token, "movetime")) {
            parseNextIntToken(&command, &goCmd.searchMoveTimeLimitMs);
        } else if (state->debugMode) {
            printf("info string Ignoring unknown go token: %s\n", token);
        }
    }

    if (searchmovesSpecified && goCmd.searchMoves.count == 0) {
        printf("bestmove 0000\n");
        fflush(stdout);
        return;
    }

    // handle go command with the parsed parameters in goCmd struct
    if (state->debugMode) {
        printf("info string Parsed go command parameters:\n");
        printf("  ponder: %d\n", goCmd.ponder);
        printf("  infiniteSearch: %d\n", goCmd.infiniteSearch);
        printf("  timeForWhiteMs: %d\n", goCmd.timeForWhiteMs);
        printf("  timeForBlackMs: %d\n", goCmd.timeForBlackMs);
        printf("  incrementForWhiteMs: %d\n", goCmd.incrementForWhiteMs);
        printf("  incrementForBlackMs: %d\n", goCmd.incrementForBlackMs);
        printf("  movesUntilNextTimeControl: %d\n",
            goCmd.movesUntilNextTimeControl);
        printf("  searchDepthLimit: %d\n", goCmd.searchDepthLimit);
        printf("  searchNodeLimit: %d\n", goCmd.searchNodeLimit);
        printf("  searchForMateInNMoves: %d\n", goCmd.searchForMateInNMoves);
        printf("  searchMoveTimeLimitMs: %d\n", goCmd.searchMoveTimeLimitMs);
        printf("  searchMoves count: %d\n", goCmd.searchMoves.count);
    }

    searchOnGoCommand(state, goCmd);
}
void handlePositionCommand(UCIState* state, const char* command)
{
    stopSearchIfRunning(state);

    command += 8; // Skip "position"
    skipWhitespace(&command);

    if (!strncmp(command, "startpos", 8) && (command[8] == '\0' || isspace((unsigned char)command[8]))) {
        bool success = fenToCBoard(START_FEN, &state->board);
        if (!success) {
            printf("info string Failed to parse start position FEN\n");
        }
        command += 8;
    } else if (!strncmp(command, "fen", 3) && (command[3] == '\0' || isspace((unsigned char)command[3]))) {
        command += 3;
        skipWhitespace(&command);

        // Find optional " moves " token as a full keyword, not as a set of chars.
        const char* moves_pos = strstr(command, " moves");
        size_t fen_length = moves_pos ? (size_t)(moves_pos - command) : strlen(command);
        while (fen_length > 0 && isspace((unsigned char)command[fen_length - 1])) {
            fen_length--;
        }

        char fen_copy[1024];
        if (fen_length >= sizeof(fen_copy)) {
            printf("info string FEN too long in position command\n");
            return;
        }
        strncpy(fen_copy, command, fen_length);
        fen_copy[fen_length] = '\0';
        bool success = fenToCBoard(fen_copy, &state->board);
        if (!success) {
            printf("info string Failed to parse FEN: %s\n", fen_copy);
        }
        command += fen_length;

        if (state->debugMode) {
            printf("info string Parsed FEN: %s\n", fen_copy);
            printf("info string Command after FEN: '%s'\n", command);
        }
    }

    // process moves if present
    skipWhitespace(&command);
    if (!strncmp(command, "moves", 5) && (command[5] == '\0' || isspace((unsigned char)command[5]))) {
        command += 5;
        skipWhitespace(&command);
        size_t move_length = strcspn(command, " \r\n");
        while (move_length > 0) {
            char move_str[16];

            if (move_length >= sizeof(move_str)) {
                printf("info string Move token too long in position command\n");
                return;
            }

            strncpy(move_str, command, move_length);
            move_str[move_length] = '\0';
            Move move = parseLongAlgebraicMove(&state->board, move_str);
            if (getFromSquare(move) != NO_SQUARE && getToSquare(move) != NO_SQUARE) {
                makeMove(&state->board, move);
            } else {
                printf("info string Invalid move in position command: %s\n", move_str);
            }
            command += move_length;
            skipWhitespace(&command);
            move_length = strcspn(command, " \r\n");
        }
    }
}
void uciLoop(void)
{
    static char line_input[8192]; // Buffer for reading input lines (max UCI
                                  // command length is 512 characters)

    UCIState state = { 0 };
    state.initialized = false;
    state.debugMode = false;
    state.ready = false;
    state.quitting = false;
    state.isSearching = false;
    bool initSuccess = fenToCBoard(START_FEN, &state.board);
    if (!initSuccess) {
        printf("info string Failed to parse start position FEN\n");
        return;
    }
    while (safeLineRead(line_input)) {
        const char* p = line_input;
        skipWhitespace(&p);

        if (*p == '\0') {
            continue; // blank line
        }
        if (!strncmp(p, "quit", 4) && (p[4] == '\0' || isspace((unsigned char)p[4]))) {
            stopSearchIfRunning(&state);
            state.quitting = true;
            break;
        }

        else if (!strncmp(p, "isready", 7) && (p[7] == '\0' || isspace((unsigned char)p[7]))) {
            printf("readyok\n");
            fflush(stdout);
            state.ready = true;
        } else if (!strncmp(p, "uci", 3) && (p[3] == '\0' || isspace((unsigned char)p[3]))) {
            initEngine();

            // Fallback if VERSION is not defined by the build system.
#ifndef VERSION
#define VERSION "dev"
#endif

            printf("id name Prophet %s\n", VERSION);
            printf("id author Nicolas Carbone\n");
            printf("option name Hash type spin default 64 min 1 max 1024\n");
            printf("option name Clear Hash type button\n");
            printf("uciok\n");
            fflush(stdout);
            state.initialized = true;
        } else if (!strncmp(p, "setoption", 9) && (p[9] == '\0' || isspace((unsigned char)p[9]))) {
            handleSetOptionCommand(&state, p);
        } else if (!strncmp(p, "ucinewgame", 10) && (p[10] == '\0' || isspace((unsigned char)p[10]))) {
            stopSearchIfRunning(&state);
            bool success = fenToCBoard(START_FEN, &state.board);

            if (!success) {
                printf("info string Failed to reset board to start position\n");
                fflush(stdout);
            }
            clearTT();
            clearSearchHeuristics();
            // Board/game state is reset via START_FEN, and search state via
            // TT/heuristic clears.
        } else if (!strncmp(p, "position", 8) && (p[8] == '\0' || isspace((unsigned char)p[8]))) {
            handlePositionCommand(&state, p);
        } else if (!strncmp(p, "go", 2) && (p[2] == '\0' || isspace((unsigned char)p[2]))) {
            handleGoCommand(&state, p);
        } else if (!strncmp(p, "stop", 4) && (p[4] == '\0' || isspace((unsigned char)p[4]))) {
            stopSearchIfRunning(&state);
        } else if (!strncmp(p, "ponderhit", 9) && (p[9] == '\0' || isspace((unsigned char)p[9]))) {
            onPonderHit();
            if (state.debugMode) {
                printf("info string ponderhit received\n");
            }
        } else if (!strncmp(p, "debug", 5) && (p[5] == '\0' || isspace((unsigned char)p[5]))) {
            p += 5;
            skipWhitespace(&p);
            if (!strncmp(p, "on", 2) && (p[2] == '\0' || isspace((unsigned char)p[2]))) {
                state.debugMode = true;
                printf("info string Debug mode enabled\n");
            } else if (!strncmp(p, "off", 3) && (p[3] == '\0' || isspace((unsigned char)p[3]))) {
                state.debugMode = false;
                printf("info string Debug mode disabled\n");
            } else {
                printf("info string Invalid debug command: %s\n", p);
            }
        } else if (!strncmp(p, "printboard", 10) && (p[10] == '\0' || isspace((unsigned char)p[10]))) {
            if (state.debugMode) {
                printBoard(&state.board);
                fflush(stdout);
            } else {
                printf("info string 'printboard' command only works in debug mode\n");
                fflush(stdout);
            }
        } else {
            if (state.debugMode) {
                printf("info string Ignoring unknown command: %s\n", p);
                fflush(stdout);
            }
        }
    }
}