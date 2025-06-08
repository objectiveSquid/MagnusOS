#pragma once

#define GET_BIT(array, bit) (array[(bit) / 8] & (1 << ((bit) % 8)))
#define SET_BIT(array, bit) (array[(bit) / 8] |= (1 << ((bit) % 8)))
#define UNSET_BIT(array, bit) (array[(bit) / 8] &= ~(1 << ((bit) % 8)))
