#include "uci/uci.h"
#include <stdio.h>
#include "engine/engine.h"
#include "movegen/move_make.h"
#include "movegen/movegen.h"
#include "board/undo.h"
#include <string.h>
#include <ctype.h>

#include "movegen/move.h"

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

int safeLineRead(char *line_input)
{
    if (fgets(line_input, 8192, stdin) == NULL)
    {
        return 0;
    }
    size_t ender_pos = strcspn(line_input, "\r\n");
    if (ender_pos < strlen(line_input))
    {
        line_input[ender_pos] = '\0'; // Remove newline characters
    }
    return 1;
}

void skipWhitespace(const char **str)
{
    while (**str && isspace((unsigned char)**str))
    {
        (*str)++;
    }
}

Square algebraicToSquare(const char *squareStr)
{
    if (strlen(squareStr) < 2)
    {
        return NO_SQUARE;
    }
    char file = squareStr[0];
    char rank = squareStr[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8')
    {
        return NO_SQUARE;
    }
    int fileIndex = file - 'a';
    int rankIndex = rank - '1';
    return (Square)(rankIndex * 8 + fileIndex);
}

Move parseLongAlgebraicMove(const CBoard *board, const char *moveStr)
{

    if (strlen(moveStr) < 4)
    {
        printf("info string Invalid move format: %s\n", moveStr);
        return (Move){.from = NO_SQUARE, .to = NO_SQUARE, .flag = 0};
    }

    Square from = algebraicToSquare(moveStr);
    Square to = algebraicToSquare(moveStr + 2);
    if (from == NO_SQUARE || to == NO_SQUARE)
    {
        printf("info string Invalid move format: %s\n", moveStr);
        return (Move){.from = NO_SQUARE, .to = NO_SQUARE, .flag = 0};
    }

    // generateLegalMoves() temporarily makes/unmakes moves internally.
    // Work on a copy here so parsing cannot accidentally perturb live board state.
    CBoard boardCopy = *board;
    MoveList moveList = generateLegalMoves(&boardCopy);

    char promotionChar = '\0';
    if (strlen(moveStr) >= 5)
    {
        promotionChar = (char)tolower((unsigned char)moveStr[4]);
    }

    for (int i = 0; i < moveList.count; i++)
    {
        if (moveList.moves[i].from == from && moveList.moves[i].to == to)
        {
            if (promotionChar == '\0')
            {
                return moveList.moves[i];
            }

            if (!move_is_promotion(moveList.moves[i]))
            {
                continue;
            }

            PieceType promoType = getPromotionPieceType(moveList.moves[i]);
            bool promotionMatches =
                (promotionChar == 'n' && promoType == KNIGHT) ||
                (promotionChar == 'b' && promoType == BISHOP) ||
                (promotionChar == 'r' && promoType == ROOK) ||
                (promotionChar == 'q' && promoType == QUEEN);

            if (promotionMatches)
            {
                return moveList.moves[i];
            }
            continue;
        }
    }
    printf("info string Move not in legal moves list: %s\n", moveStr);
    return (Move){.from = NO_SQUARE, .to = NO_SQUARE, .flag = 0};
}

void handlePositionCommand(UCIState *state, const char *command)
{
    command += 8; // Skip "position"
    skipWhitespace(&command);

    if (!strncmp(command, "startpos", 8) && (command[8] == '\0' || isspace((unsigned char)command[8])))
    {
        state->board = fenToCBoard(START_FEN);
        command += 8;
    }
    else if (!strncmp(command, "fen", 3) && (command[3] == '\0' || isspace((unsigned char)command[3])))
    {
        command += 3;
        skipWhitespace(&command);

        // Find optional " moves " token as a full keyword, not as a set of chars.
        const char *moves_pos = strstr(command, " moves");
        size_t fen_length = moves_pos ? (size_t)(moves_pos - command) : strlen(command);
        while (fen_length > 0 && isspace((unsigned char)command[fen_length - 1]))
        {
            fen_length--;
        }

        char fen_copy[1024];
        if (fen_length >= sizeof(fen_copy))
        {
            printf("info string FEN too long in position command\n");
            return;
        }
        strncpy(fen_copy, command, fen_length);
        fen_copy[fen_length] = '\0';
        state->board = fenToCBoard(fen_copy);
        command += fen_length;

        if (state->debugMode)
        {
            printf("info string Parsed FEN: %s\n", fen_copy);
            printf("info string Command after FEN: '%s'\n", command);
        }
    }

    // process moves if present
    skipWhitespace(&command);
    if (!strncmp(command, "moves", 5) && (command[5] == '\0' || isspace((unsigned char)command[5])))
    {
        command += 5;
        skipWhitespace(&command);
        size_t move_length = strcspn(command, " \r\n");
        while (move_length > 0)
        {
            char move_str[16];

            if (move_length >= sizeof(move_str))
            {
                printf("info string Move token too long in position command\n");
                return;
            }

            strncpy(move_str, command, move_length);
            move_str[move_length] = '\0';
            Move move = parseLongAlgebraicMove(&state->board, move_str);
            if (move.from != NO_SQUARE && move.to != NO_SQUARE)
            {
                makeMove(&state->board, move);
            }
            else
            {
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

    static char line_input[8192]; // Buffer for reading input lines (max UCI command length is 512 characters)

    UCIState state = {0};
    state.initialized = false;
    state.debugMode = false;
    state.ready = false;
    state.quitting = false;
    state.board = fenToCBoard(START_FEN); // Initialize board to starting position
    while (safeLineRead(line_input))
    {
        const char *p = line_input;
        skipWhitespace(&p);

        if (*p == '\0')
        {
            continue; // blank line
        }
        if (!strncmp(p, "quit", 4) && (p[4] == '\0' || isspace((unsigned char)p[4])))
        {
            state.quitting = true;
            break;
        }

        else if (!strncmp(p, "isready", 7) && (p[7] == '\0' || isspace((unsigned char)p[7])))
        {
            // TODO: research how to handle isready if operations are still pending (e.g. if we're still thinking or doing a long operation)
            if (state.initialized)
            {
                printf("readyok\n");
                state.ready = true;
            }
            else
            {
                printf("info string Engine not initialized. Please send 'uci' command first.\n");
            }
        }
        else if (!strncmp(p, "uci", 3) && (p[3] == '\0' || isspace((unsigned char)p[3])))
        {
            printf("id name Prophet\n");
            printf("id author Nicolas Carbone\n");
            printf("uciok\n");
            state.initialized = true;
        }
        else if (!strncmp(p, "setoption", 9) && (p[9] == '\0' || isspace((unsigned char)p[9])))
        {
            printf("info string No options available\n");
        }
        else if (!strncmp(p, "ucinewgame", 10) && (p[10] == '\0' || isspace((unsigned char)p[10])))
        {
            state.board = fenToCBoard(START_FEN);
        }
        else if (!strncmp(p, "position", 8) && (p[8] == '\0' || isspace((unsigned char)p[8])))
        {
            handlePositionCommand(&state, p);
        }
        else if (!strncmp(p, "go", 2) && (p[2] == '\0' || isspace((unsigned char)p[2])))
        {
            printf("info string 'go' command received, handling unfinished\n");
        }
        else if (!strncmp(p, "stop", 4) && (p[4] == '\0' || isspace((unsigned char)p[4])))
        {
            printf("info string 'stop' command received, handling unfinished\n");
        }
        else if (!strncmp(p, "ponderhit", 9) && (p[9] == '\0' || isspace((unsigned char)p[9])))
        {
            printf("info string 'ponderhit' command received, handling unfinished\n");
        }
        else if (!strncmp(p, "debug", 5) && (p[5] == '\0' || isspace((unsigned char)p[5])))
        {
            p += 5;
            skipWhitespace(&p);
            if (!strncmp(p, "on", 2) && (p[2] == '\0' || isspace((unsigned char)p[2])))
            {
                state.debugMode = true;
                printf("info string Debug mode enabled\n");
            }
            else if (!strncmp(p, "off", 3) && (p[3] == '\0' || isspace((unsigned char)p[3])))
            {
                state.debugMode = false;
                printf("info string Debug mode disabled\n");
            }
            else
            {
                printf("info string Invalid debug command: %s\n", p);
            }
        }
        else if (!strncmp(p, "printboard", 10) && (p[10] == '\0' || isspace((unsigned char)p[10])))
        {
            printBoard(&state.board);
        }
        else
        {
            printf("Unknown command: %s\n", p);
        }

        // else if (!strncmp(p, "debug", 5))
        // {
        //     p += 5;
        //     skipWhitespace(&p);
        //     if (!strncmp(p, "on", 2) && (p[2] == '\0' || isspace((unsigned char)p[2])))
        //     {
        //         state.debugMode = true;
        //         printf("info string Debug mode enabled\n");
        //     }
        //     else if (!strncmp(p, "off", 3) && (p[3] == '\0' || isspace((unsigned char)p[3])))
        //     {
        //         state.debugMode = false;
        //         printf("info string Debug mode disabled\n");
        //     }
        //     else
        //     {
        //         printf("info string Invalid debug command: %s\n", p);
        //     }
        // }
        // else if (!strncmp(p, "position", 8) && (p[8] == '\0' || isspace((unsigned char)p[8])))
        // {
        //     p += 8;
        //     skipWhitespace(&p);
        //     if (!strncmp(p, "startpos", 8) && (p[8] == '\0' || isspace((unsigned char)p[8])))
        //     {
        //         state.board = fenToCBoard(START_FEN);
        //     }
        //     else
        //     {
        //         // Prefer silent ignore unless debug mode is on
        //         if (state.debugMode)
        //         {
        //             printf("info string Unknown command: %s\n", p);
        //         }
        //     }
        // }
    }
}