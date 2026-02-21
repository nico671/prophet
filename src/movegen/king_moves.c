#include "movegen/king_moves.h"
#include "attacks/constant_attacks.h"
#include "core/bitboard.h"
#include "board/cboard.h"

void genAllPseudoLegalKingNonCastlingMoves(CBoard *board, MoveList *moveList)
{
    Color sideToMove = board->sideToMove;
    Bitboard king = (sideToMove == WHITE) ? board->whiteKing : board->blackKing;
    Bitboard opponentPieces = (sideToMove == WHITE) ? board->blackPieces : board->whitePieces;
    if (king)
    {
        Square from = bitboardPopLSB(&king);
        Bitboard attacks = getKingAttacks(from);

        Bitboard captures = attacks & opponentPieces;
        while (captures)
        {
            Square to = bitboardPopLSB(&captures);
            Move move = MAKE_CAPTURE(from, to);
            moveList->moves[moveList->count++] = move;
        }

        Bitboard quietMoves = attacks & ~board->allPieces;
        while (quietMoves)
        {
            Square to = bitboardPopLSB(&quietMoves);
            Move move = MAKE_MOVE(from, to);
            moveList->moves[moveList->count++] = move;
        }
    }
}

void genAllPseudoLegalKingCastlingMoves(CBoard *board, MoveList *moveList)
{
    // handle white castling
    if (board->sideToMove == WHITE)
    {
        if (CHECK_BIT(board->castlingRights, 3))
        {
            if ((bitboardLSBIndex(board->whiteKing) == E1) &&
                (bitboardIsBitSet(board->whiteRooks, H1)) &&
                !(bitboardIsBitSet(board->allPieces, F1)) &&
                !(bitboardIsBitSet(board->allPieces, G1)))
            {

                Move move = MAKE_CASTLE_KING(E1, G1);
                moveList->moves[moveList->count++] = move;
            }
        }
        if (CHECK_BIT(board->castlingRights, 2))
        {
            if ((bitboardLSBIndex(board->whiteKing) == E1) &&
                (bitboardIsBitSet(board->whiteRooks, A1)) &&
                !(bitboardIsBitSet(board->allPieces, D1)) &&
                !(bitboardIsBitSet(board->allPieces, C1)) &&
                !(bitboardIsBitSet(board->allPieces, B1)))
            {

                Move move = MAKE_CASTLE_QUEEN(E1, C1);
                moveList->moves[moveList->count++] = move;
            }
        }
    }
    // handle black castling
    else
    {
        if (CHECK_BIT(board->castlingRights, 1))
        {
            if ((bitboardLSBIndex(board->blackKing) == E8) &&
                (bitboardIsBitSet(board->blackRooks, H8)) &&
                !(bitboardIsBitSet(board->allPieces, F8)) &&
                !(bitboardIsBitSet(board->allPieces, G8)))
            {

                Move move = MAKE_CASTLE_KING(E8, G8);
                moveList->moves[moveList->count++] = move;
            }
        }
        if (CHECK_BIT(board->castlingRights, 0))
        {
            if ((bitboardLSBIndex(board->blackKing) == E8) &&
                (bitboardIsBitSet(board->blackRooks, A8)) &&
                !(bitboardIsBitSet(board->allPieces, D8)) &&
                !(bitboardIsBitSet(board->allPieces, C8)) &&
                !(bitboardIsBitSet(board->allPieces, B8)))
            {

                Move move = MAKE_CASTLE_QUEEN(E8, C8);
                moveList->moves[moveList->count++] = move;
            }
        }
    }
}

void genAllPseudoLegalKingMoves(CBoard *board, MoveList *moveList)
{
    genAllPseudoLegalKingNonCastlingMoves(board, moveList);
    genAllPseudoLegalKingCastlingMoves(board, moveList);
}
