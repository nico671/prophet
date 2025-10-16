
#include "board.h"

// Recompute occupancy bitboards based on individual piece bitboards
void recomputeOccupancies(CBoard *board)
{
    board->whitePieces = board->whiteBishops | board->whiteKing | board->whiteKnights | board->whitePawns | board->whiteQueens | board->whiteRooks; // white pieces are the OR of all white piece bitboards
    board->blackPieces = board->blackBishops | board->blackKing | board->blackKnights | board->blackPawns | board->blackQueens | board->blackRooks; // black pieces are the OR of all black piece bitboards
    board->allPieces = board->whitePieces | board->blackPieces;                                                                                     // all pieces are the OR of white and black pieces
}
// Bitboard consistency
// Exactly one king per side: whitePawns and blackPawns each have exactly one bit set
// Piece counts within legal bounds: max 8 pawns, 2 rooks/bishops/knights, 1 queen per side
// Pawn placement rules
// No pawns on ranks 1 or 8 (they must have promoted)
// Pawn count consistency: if more than 8 pawns total for one side, some pawns promoted, so missing pieces should account for this
// King safety and check rules
// At most one side can be in check (the side not to move)
// If the side not to move is in check, that's an illegal position (you can't move into check)
// Castling rights consistency
// If white king-side castling is allowed: white king on e1, white rook on h1
// If white queen-side castling is allowed: white king on e1, white rook on a1
// Same logic for black castling rights with king on e8, rooks on a8/h8
// No castling rights if the king or relevant rook has moved (can't validate this from position alone, but check basic placement)
// En passant validity
// If epSquare != 64: the square must be on rank 3 (for white to capture) or rank 6 (for black to capture)
// The square behind the en passant target must contain an enemy pawn
// The capturing side must have a pawn adjacent to the en passant target pawn
// The en passant capture must not leave the capturing side's king in check
// Game state bounds
// whiteToMove is either true or false (boolean constraint)
// halfmoveClock should be reasonable (0-100+ for fifty-move rule)
// fullmoveNumber should be positive (starts at 1)
// epSquare should be 0-63 or the sentinel value 64
bool validateBoard(CBoard *board)
{
    // No overlapping pieces: each square occupied by at most one piece across all 12 piece bitboards
    if ((board->allPieces != (board->whitePieces | board->blackPieces)) ||
        (board->blackPieces != (board->blackBishops | board->blackKing | board->blackKnights | board->blackPawns | board->blackQueens | board->blackRooks)) ||
        (board->whitePieces != (board->whiteBishops | board->whiteKing | board->whiteKnights | board->whitePawns | board->whiteQueens | board->whiteRooks)))
    {
        return false;
    }

    // Add more validations here...

    return true; // Missing this!
}