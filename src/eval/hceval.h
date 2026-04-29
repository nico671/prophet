#ifndef HCEVAL_H
#define HCEVAL_H

typedef struct CBoard CBoard;
#define PAWN_VALUE 100
#define KNIGHT_VALUE 300
#define BISHOP_VALUE 325
#define ROOK_VALUE 500
#define QUEEN_VALUE 900
#define KING_VALUE 10000 // extremely high value so the engine knows to protect it

/**
 * @brief Initializes the evaluation function, precomputing any necessary tables or parameters. This function is idempotent and can be safely called multiple times without adverse effects. It must be called before any calls to evaluateBoard to ensure that the evaluation function is properly initialized and ready for use.
 *
 */
void hcEvalInit(void);

/**
 * @brief Evaluates a chess board position.
 *
 * @param board The board to evaluate.
 * @return int The evaluation score (positive for white advantage, negative for black advantage).
 */
int evaluateBoard(const CBoard* board);

#endif // HCEVAL_H