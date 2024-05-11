#ifndef _FORTUNA_UTILS_STUBS_
#define _FORTUNA_UTILS_STUBS_

#include <uk/init.h>
#include <stdint.h>
#define __NEED_size_t
#include <nolibc-internal/shareddefs.h>

/* Ignores the _data argument intended for _func. */
#define	SYSINIT(uniq, subs, order, _func, _data)	\
uk_early_initcall(_func, 0x0)

void explicit_bzero(void *p, size_t n);
int timingsafe_bcmp(const void *b1, const void *b2, size_t n);

#endif // _FORTUNA_UTILS_STUBS_
