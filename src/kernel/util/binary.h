#pragma once

#define FLAG_SET(x, flag) (x |= (flag))
#define FLAG_UNSET(x, flag) (x &= ~(flag))

#define BIT_SET(x, bit) (x |= (1 << bit))
#define BIT_UNSET(x, bit) (x &= ~(1 << bit))
#define BIT_IS_SET(x, bit) (x & (1 << bit))
#define BIT_TOGGLE(x, bit) (x ^= (1 << bit))
