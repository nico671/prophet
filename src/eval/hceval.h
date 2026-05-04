#ifndef HCEVAL_H
#define HCEVAL_H

typedef struct CBoard CBoard;
#define HC_PAWN_VALUE 100
#define HC_KNIGHT_VALUE 300
#define HC_BISHOP_VALUE 325
#define HC_ROOK_VALUE 500
#define HC_QUEEN_VALUE 900
#define HC_KING_VALUE 10000 // extremely high value so the engine knows to protect it

/**
 * @brief Initializes the evaluation function, precomputing any necessary tables or parameters. This function is idempotent and can be safely called multiple times without adverse effects. It must be called before any calls to evaluate_cboard to ensure that the evaluation function is properly initialized and ready for use.
 *
 */
void hc_eval_init(void);

/**
 * @brief Evaluates a chess board position.
 *
 * @param board The board to evaluate.
 * @return int The evaluation score (positive for white advantage, negative for black advantage).
 */
int evaluate_cboard(const CBoard* board);

#endif // HCEVAL_H