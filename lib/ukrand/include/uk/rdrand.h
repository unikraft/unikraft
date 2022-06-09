#ifndef __UK_RDRAND__
#define __UK_RDRAND__

#include <sys/types.h>
#include <uk/arch/types.h>

#define cpuid_macro(ax, bx, cx, dx, func_1, func_2) do { \
		__asm__ __volatile__ ( \
			"cpuid" \
			: "=a" (ax), "=b" (bx), \
			  "=c" (cx), "=d" (dx) \
			: "a" (func_1), "c" (func_2) \
		); \
		} while (0)

size_t uk_rdrand_generate_bytes(void *buf, size_t buflen);

static inline uint8_t uk_hwrand_rdrand(__u32 *val)
{	
	__u8 success;
	
	__asm__ volatile(
		"rdrand %0 ; setc %1"
		: "=r" (*val), "=qm" (success)
	);

	return success;
}

static inline uint8_t uk_hwrand_rdseed(__u32 *val) {
	__u8 success;
	
	__asm__ volatile(
		"rdseed %0 ; setc %1"
		: "=r" (*val), "=qm" (success)
	);

	return success;
}


#endif