#include <stdint.h>
#include <stdbool.h>
// A1 = LSB, H8 = MSB, little-endian rank-file mapping
typedef uint64_t Bitboard;

typedef struct CBoard
{
    // White Piece bitboards
    Bitboard whitePawns;
    Bitboard whiteKnights;
    Bitboard whiteBishops;
    Bitboard whiteRooks;
    Bitboard whiteQueens;
    Bitboard whiteKing;

    // Black Piece bitboards
    Bitboard blackPawns;
    Bitboard blackKnights;
    Bitboard blackBishops;
    Bitboard blackRooks;
    Bitboard blackQueens;
    Bitboard blackKing;

    // Occupancy bitboards
    Bitboard whitePieces; // all white pieces
    Bitboard blackPieces; // all black pieces
    Bitboard allPieces;   // all pieces

    // Game state info
    bool whiteToMove;
    bool whiteCanCastleKingside;
    bool whiteCanCastleQueenside;
    bool blackCanCastleKingside;
    bool blackCanCastleQueenside;
    uint8_t epSquare;        // square index (0-63) or 64 if no en passant available
    uint16_t halfmoveClock;  // for fifty-move rule
    uint16_t fullmoveNumber; // starts at 1, incremented after Black's move
} CBoard;

void recomputeOccupancies(CBoard *board);

bool validateBoard(CBoard *board);