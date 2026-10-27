#ifndef __UK_PLAT_NATIVE_ARCH_TLB_H__
#define __UK_PLAT_NATIVE_ARCH_TLB_H__

#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

#if CONFIG_LIBUKPLAT_NATIVE_TLB

static inline void uk_plat_native_tlb_flush_entry(__vaddr_t vaddr)
{
	__asm__ __volatile__("sfence.vma %0, x0" : : "r" (vaddr) : "memory");
}

static inline void uk_plat_native_tlb_flush(void)
{
	__asm__ __volatile__("sfence.vma x0, x0" ::: "memory");
}

#endif /* CONFIG_LIBUKPLAT_NATIVE_TLB */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_TLB_H__ */
