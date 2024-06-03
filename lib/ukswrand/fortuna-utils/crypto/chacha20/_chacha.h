/*
 * Public domain.
 */

#ifndef _CHACHA_H
#define _CHACHA_H

#define __NEED_BSD_TYPES
#include <nolibc-internal/shareddefs.h>

struct chacha_ctx {
	u_int input[16];
};

#endif
