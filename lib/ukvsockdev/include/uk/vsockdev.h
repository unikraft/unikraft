#ifndef __UK_VSOCKDEV__
#define __UK_VSOCKDEV__

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <uk/assert.h>
#include <uk/list.h>
#include <uk/errptr.h>

#include "vsockdev_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get a reference to the Unikraft Vsock Device.
 *
 * @return
 *	- NULL: device not found
 *	- (struct uk_vsockdev *): reference to the vsock device
 */
struct uk_vsockdev *uk_vsockdev_get();

/**
 * Init uk vsock device
 */
int uk_vsockdev_init();

#ifdef __cplusplus
}
#endif

#endif /* __UK_VSOCKDEV__ */
