#ifndef PROPHET_SEE_H
#define PROPHET_SEE_H

#include "chess/board/cboard.h"
#include "chess/movegen/move.h"

/**
 * @brief Returns the material result of the capture sequence on a square.
 *
 * The result is positive when the side to move benefits from the exchange.
 * Only legal recaptures are considered.
 */
int see_capture(const CBoard* board, Move move);

#endif // PROPHET_SEE_H
