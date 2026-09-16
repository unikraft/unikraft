#ifndef __UK_DEBUG_ARCH_CRASH_H__
#define __UK_DEBUG_ARCH_CRASH_H__

#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

extern __bool _uk_crash_explicit;

#define uk_crash_trigger()						\
	do {								\
		_uk_crash_explicit = 1;					\
		__asm__ __volatile__("ebreak" : : : "memory");		\
		__builtin_unreachable();				\
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* __UK_DEBUG_ARCH_CRASH_H__ */
