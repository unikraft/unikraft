#ifndef __UK_HWRAND__
#define __UK_HWRAND__

#include <sys/types.h>

#define cpuid_macro(ax, bx, cx, dx, func_1, func_2) do { \
		__asm__ __volatile__ ( \
			"cpuid" \
			: "=a" (ax), "=b" (bx), \
			  "=c" (cx), "=d" (dx) \
			: "a" (func_1), "c" (func_2) \
		); \
		} while (0)

struct CPUID_info {
	unsigned int EAX;
	unsigned int EBX;
	unsigned int ECX;
	unsigned int EDX;
};

/*
TODO remove defintion of has_RDRAND once we integrate ukcpuid library
*/
int has_RDRAND(void);

ssize_t uk_hwrand_generate_bytes(void *buf, size_t buflen);

static inline __u32 uk_hwrand_randr(uint8_t *success)
{	
	__u32 val;
	__asm__ volatile(
		"rdrand %0 ; setc %1"
		: "=r" (val), "=qm" (*success)
	);

	return val;
}

#endif