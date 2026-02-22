#include "movegen/movegen.h"
#include "movegen/pawn_moves.h"
#include "movegen/knight_moves.h"
#include "movegen/sliding_moves.h"
#include "movegen/king_moves.h"
#include "movegen/move_make.h"
#include "attacks/constant_attacks.h"
#include "attacks/sliding_attacks.h"
#include "core/bitboard.h"
#include "board/cboard.h"
#include <stdbool.h>

void genAllPseudoLegalMoves(CBoard *board, MoveList *moveList) // TODO: refactor movegen to have less reused code everywhere, all individual piece gen is abt the same minus pawns and kings
{
    genAllPseudoLegalPawnMoves(board, moveList);
    genAllPseudoLegalKnightMoves(board, moveList);
    genAllPseudoLegalBishopMoves(board, moveList);
    genAllPseudoLegalRookMoves(board, moveList);
    genAllPseudoLegalQueenMoves(board, moveList);
    genAllPseudoLegalKingMoves(board, moveList);
}

void initMoveList(MoveList *moveList)
{
    moveList->count = 0;
}

bool isSquareAttacked(CBoard *board, Square square, Color attackerColor)
{
    // Check for pawn attacks
    // We need to check if pawns of attackerColor can attack this square
    // If white pawns attack diagonally upward, we need to check squares diagonally downward
    // So we use the OPPOSITE color's attack pattern (FLIPPED, like in genEnPassantPawnMoves)
    Bitboard pawnAttacks = (attackerColor == WHITE)
                               ? getPawnAttacks(square, BLACK)  // FLIPPED
                               : getPawnAttacks(square, WHITE); // FLIPPED
    Bitboard attackerPawns = (attackerColor == WHITE) ? board->whitePawns : board->blackPawns;
    if (pawnAttacks & attackerPawns)
        return true;

    // Check for knight attacks
    Bitboard knightAttacks = getKnightAttacks(square);
    Bitboard attackerKnights = (attackerColor == WHITE) ? board->whiteKnights : board->blackKnights;
    if (knightAttacks & attackerKnights)
        return true;

    // Check for bishop/queen attacks
    Bitboard bishopAttacks = getBishopAttacks(square, board->allPieces);
    Bitboard attackerBishops = (attackerColor == WHITE) ? board->whiteBishops : board->blackBishops;
    Bitboard attackerQueens = (attackerColor == WHITE) ? board->whiteQueens : board->blackQueens;
    if (bishopAttacks & (attackerBishops | attackerQueens))
        return true;

    // Check for rook/queen attacks
    Bitboard rookAttacks = getRookAttacks(square, board->allPieces);
    Bitboard attackerRooks = (attackerColor == WHITE) ? board->whiteRooks : board->blackRooks;
    if (rookAttacks & (attackerRooks | attackerQueens))
        return true;

    // Check for king attacks
    Bitboard kingAttacks = getKingAttacks(square);
    Bitboard attackerKing = (attackerColor == WHITE) ? board->whiteKing : board->blackKing;
    if (kingAttacks & attackerKing)
        return true;

    return false;
}

bool isKingInCheck(CBoard *board, Color side)
{
    Square kingSquare = (side == WHITE) ? bitboardLSBIndex(board->whiteKing) : bitboardLSBIndex(board->blackKing);
    Color opponentColor = (side == WHITE) ? BLACK : WHITE;
    return isSquareAttacked(board, kingSquare, opponentColor);
}

MoveList generateLegalMoves(CBoard *board)
{
    MoveList pseudoLegalMoves;
    initMoveList(&pseudoLegalMoves);
    genAllPseudoLegalMoves(board, &pseudoLegalMoves);

    MoveList legalMoves;
    initMoveList(&legalMoves);

    for (int i = 0; i < pseudoLegalMoves.count; i++)
    {
        Move move = pseudoLegalMoves.moves[i];

        // Special handling for castling
        if (move_is_castling(move))
        {
            Color side = board->sideToMove;
            Color opponent = (side == WHITE) ? BLACK : WHITE;
            // Square kingFrom = FROM_SQ(move);

            // Cannot castle if in check
            if (isKingInCheck(board, side))
            {
                continue;
            }

            // Check squares the king moves through
            if (MOVE_FLAG(move) == KINGSIDE_CASTLE)
            {
                Square throughSquare = (side == WHITE) ? F1 : F8;
                if (isSquareAttacked(board, throughSquare, opponent))
                {
                    continue;
                }
            }
            else // QUEENSIDE_CASTLE
            {
                Square throughSquare = (side == WHITE) ? D1 : D8;
                if (isSquareAttacked(board, throughSquare, opponent))
                {
                    continue;
                }
            }
        }

        // Normal legality check for all moves (including castling destination)
        UndoInfo undoInfo = makeMove(board, move);
        if (!isKingInCheck(board, (board->sideToMove == WHITE) ? BLACK : WHITE))
        {
            legalMoves.moves[legalMoves.count++] = move;
        }
        unmakeMove(board, move, undoInfo);
    }

    return legalMoves;
}