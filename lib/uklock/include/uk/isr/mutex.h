#include <uk/mutex.h>

/*
 * NOTE: uk_mutex_trylock() is a static inline function, so it will
 * use the same compilation flags as the unit where it is used
 */

#define uk_mutex_trylock_isr(x) uk_mutex_trylock(x)
