#ifndef __ENTROPY__
#define __ENTROPY__

#include <uk/plat/lcpu.h>
#include <sys/types.h>

size_t uk_entropy_generate_bytes(void *buf, size_t buflen);
int uk_entropy_init(void);

#endif