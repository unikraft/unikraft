#ifndef __UK_HWRAND__
#define __UK_HWRAND__

#include <sys/types.h>

size_t uk_hwrand_generate_bytes(void *buf, size_t buflen);
int _uk_hwrand_init(void);
#endif