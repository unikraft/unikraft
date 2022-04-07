#ifndef __RD_RAND__
#define __RD_RAND__

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

#endif
