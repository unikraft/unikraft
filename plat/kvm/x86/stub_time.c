/*
 * Author: Dragos Petre <dragos.petre27@gmail.com>
 *
 * This module is a stub for time.c to be used to build the minimum image.
 */

#include <stdlib.h>
#include <uk/plat/time.h>

/* Stub call */
__nsec ukplat_monotonic_clock(void)
{
	return 0;
}

/* Stub call */
__nsec ukplat_wall_clock(void)
{
	return 0;
}

/* Stub call */
static int timer_handler(void *arg __unused)
{
	return 1;
}

/* Stub call */
void ukplat_time_init(void)
{
}

/* Stub call */
void ukplat_time_fini(void)
{
}

/* Stub call */
uint32_t ukplat_time_get_irq(void)
{
	return 0;
}
