/*
 * Author: Dragos Petre <dragos.petre27@gmail.com>
 *
 * This module is a stub for lcpu.c to be used to build the minimum image.
 */

#include <uk/plat/common/lcpu.h>

struct lcpu lcpus[CONFIG_UKPLAT_LCPU_MAXCOUNT];

int lcpu_init(struct lcpu *this_lcpu)
{
    /*
     * Stub for the same function from plat/common/lcpu.c
     */
    this_lcpu = NULL;

    return 0;
}