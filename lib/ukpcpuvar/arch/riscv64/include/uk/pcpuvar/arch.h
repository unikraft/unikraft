#ifndef __UK_PCPUVAR_H__
#error "Do not include this header directly"
#endif

#include <uk/config.h>
#include <uk/essentials.h>

#if CONFIG_UKPLAT_CPU_MAXCOUNT != 1
#error "RISC-V current pcpuvar backend only supports single-hart bring-up"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

#define __uk_pcpuvar_arch_current_get(_sym)				\
	(_sym)

#define __uk_pcpuvar_arch_current_set(_sym, _val)			\
	do {								\
		(_sym) = (_val);						\
	} while (0)

#define __uk_pcpuvar_arch_current_member_get(_sym, _member)		\
	((_sym)._member)

#define __uk_pcpuvar_arch_current_member_set(_sym, _member, _val)	\
	do {								\
		(_sym)._member = (_val);					\
	} while (0)

#define __uk_pcpuvar_arch_current_ptr_get(_sym)				\
	(&(_sym))

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */
