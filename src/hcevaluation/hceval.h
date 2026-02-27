#ifndef HCEVAL_H
#define HCEVAL_H

typedef struct CBoard CBoard;
#define PAWN_VALUE 100
#define KNIGHT_VALUE 300
#define BISHOP_VALUE 325
#define ROOK_VALUE 500
#define QUEEN_VALUE 900
#define KING_VALUE 10000 // High value so the engine knows to protect it

// Initializes evaluation tables (safe to call multiple times).
void hcEvalInit(void);

/**
 * @brief Evaluates a chess board position.
 *
 * @param board The board to evaluate.
 * @return int The evaluation score (positive for white advantage, negative for black advantage).
 */
int evaluateBoard(const CBoard *board);

#endif // HCEVAL_H