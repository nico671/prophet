#ifndef BITOPS_H
#define BITOPS_H
// generic bitfield ops, not for bitboards but useful for other bitfield manipulations
#define BIT_MASK(pos) (1ULL << (pos))
#define SET_BIT(var, pos) ((var) |= BIT_MASK(pos))
#define CLEAR_BIT(var, pos) ((var) &= ~BIT_MASK(pos))
#define CHECK_BIT(var, pos) (!!((var) & BIT_MASK(pos)))
#endif // BITOPS_H