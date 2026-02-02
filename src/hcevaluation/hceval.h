#ifndef HCEVAL_H
#define HCEVAL_H

typedef struct CBoard CBoard;
#define PAWN_VALUE 100
#define KNIGHT_VALUE 300
#define BISHOP_VALUE 325
#define ROOK_VALUE 500
#define QUEEN_VALUE 900
#define KING_VALUE 10000 // High value so the engine knows to protect it

int evaluateBoard(const CBoard *board);

#endif // HCEVAL_H