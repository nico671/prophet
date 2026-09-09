#ifndef PROPHET_CHESS_TYPES_H
#define PROPHET_CHESS_TYPES_H

/*
+---+----+----+----+----+----+----+----+----+
|   | a  | b  | c  | d  | e  | f  | g  | h  |
+---+----+----+----+----+----+----+----+----+
| 8 | 56 | 57 | 58 | 59 | 60 | 61 | 62 | 63 |
| 7 | 48 | 49 | 50 | 51 | 52 | 53 | 54 | 55 |
| 6 | 40 | 41 | 42 | 43 | 44 | 45 | 46 | 47 |
| 5 | 32 | 33 | 34 | 35 | 36 | 37 | 38 | 39 |
| 4 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 |
| 3 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 |
| 2 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 |
| 1 |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |
+---+----+----+----+----+----+----+----+----+
*/

/**
 * @brief Enum for chess squares, representing each square on the
 * board with a unique integer value. Consistent with little-endian
 * rank-file mapping used in bitboards.
 *
 */
typedef enum {
    A1, // 0
    B1,
    C1,
    D1,
    E1,
    F1,
    G1,
    H1,
    A2,
    B2,
    C2,
    D2,
    E2,
    F2,
    G2,
    H2,
    A3,
    B3,
    C3,
    D3,
    E3,
    F3,
    G3,
    H3,
    A4,
    B4,
    C4,
    D4,
    E4,
    F4,
    G4,
    H4,
    A5,
    B5,
    C5,
    D5,
    E5,
    F5,
    G5,
    H5,
    A6,
    B6,
    C6,
    D6,
    E6,
    F6,
    G6,
    H6,
    A7,
    B7,
    C7,
    D7,
    E7,
    F7,
    G7,
    H7,
    A8,
    B8,
    C8,
    D8,
    E8,
    F8,
    G8,
    H8 // 63
} Square;

/**
 * @brief Constant representing an invalid square
 */
#define NO_SQUARE 64

/**
 * @brief Enum for player colors in chess. 0=WHITE, BLACK=1
 */
typedef enum { WHITE = 0, BLACK = 1 } Color;

/**
 * @brief Flips a square vertically (A1 <-> A8).
 */
static inline Square square_flip_vertical(Square square)
{
    return (Square)(square ^ 56);
}

/**
 * @brief Returns the opposing color.
 */
static inline Color color_opposite(Color color)
{
    return (Color)(color ^ 1);
}

/**
 * @brief Enum for chess piece types. The piece types are assigned
 * integer values starting from 0 for NO_PIECE up to 6 for KING.
 *
 */
typedef enum {
    NO_PIECE = 0,
    PAWN     = 1,
    KNIGHT   = 2,
    BISHOP   = 3,
    ROOK     = 4,
    QUEEN    = 5,
    KING     = 6
} PieceType;
#endif // CHESS_TYPES_H
