#ifndef __ENTROPY__
#define __ENTROPY__

#include <uk/plat/lcpu.h>
#include <sys/types.h>

size_t uk_entropy_generate_bytes(void *buf, size_t buflen);
void uk_add_network_randomness(__u32 value);
void uk_add_interrupt_randomness(int irq);
int uk_entropy_init(void);
__u32 uk_entropy_get_estimated_entropy(void);

#endif