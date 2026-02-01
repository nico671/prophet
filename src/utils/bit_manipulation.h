#ifndef BIT_MANIPULATION_H
#define BIT_MANIPULATION_H

#define SET_BIT(var, pos) ((var) |= (1ULL << (pos)))
#define CLEAR_BIT(var, pos) ((var) &= ~(1ULL << (pos)))
#define CHECK_BIT(var, pos) (!!((var) & (1ULL << (pos))))
#define TOGGLE_BIT(var, pos) ((var) ^= (1ULL << (pos)))

#endif // BIT_MANIPULATION_H