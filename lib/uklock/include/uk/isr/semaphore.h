#include <uk/semaphore.h>

/*
 * NOTE: uk_semaphore_down_try() is a static inline function, so it will
 * use the same compilation flags as the unit where it is used
 */

#define uk_semaphore_down_try_isr(x) uk_semaphore_down_try(x)
