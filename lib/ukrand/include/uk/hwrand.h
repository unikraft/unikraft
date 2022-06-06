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

ssize_t uk_hwrand_generate_bytes(void *buf, size_t buflen);

static inline uint8_t uk_hwrand_randr(__u32 *val)
{	
	__u8 success;
	
	__asm__ volatile(
		"rdrand %0 ; setc %1"
		: "=r" (*val), "=qm" (success)
	);

	return success;
}

static inline uint8_t uk_hwrand_seed(__u32 *val) {
	__u8 success;
	
	__asm__ volatile(
		"rdseed %0 ; setc %1"
		: "=r" (*val), "=qm" (success)
	);

	return success;
}


#endif