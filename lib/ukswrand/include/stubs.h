#ifndef _FORTUNA_UTILS_STUBS_
#define _FORTUNA_UTILS_STUBS_

#include <stdint.h>
#define __NEED_size_t
#include <nolibc-internal/shareddefs.h>

void explicit_bzero(void *p, size_t n);
int timingsafe_bcmp(const void *b1, const void *b2, size_t n);

#endif // _FORTUNA_UTILS_STUBS_
