#ifndef __UK_PLAT_NATIVE_ARCH_SYSCTX_H__
#define __UK_PLAT_NATIVE_ARCH_SYSCTX_H__

#include <uk/arch/types.h>
#include <uk/config.h>
#include <uk/essentials.h>

#define UK_PLAT_NATIVE_RISCV64_SYSCTX_OFFSETOF_TP		0

#define UK_PLAT_NATIVE_SYSCTX_SIZE				16
#define UK_PLAT_NATIVE_SYSCTX_OFFSETOF_TLSP			\
	UK_PLAT_NATIVE_RISCV64_SYSCTX_OFFSETOF_TP

#if CONFIG_LIBUKPLAT_NATIVE_SYSCTX
#define UK_PLAT_NATIVE_SYSCTX_LOAD_FNSYM				\
	uk_plat_native_sysctx_load
#define UK_PLAT_NATIVE_SYSCTX_STORE_FNSYM				\
	uk_plat_native_sysctx_store
#endif /* CONFIG_LIBUKPLAT_NATIVE_SYSCTX */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uk_plat_native_sysctx {
	__uptr tp;
	__u8 pad[8];
};

UK_CTASSERT(sizeof(struct uk_plat_native_sysctx) == UK_PLAT_NATIVE_SYSCTX_SIZE);
UK_CTASSERT(__offsetof(struct uk_plat_native_sysctx, tp) ==
	    UK_PLAT_NATIVE_SYSCTX_OFFSETOF_TLSP);

#if CONFIG_LIBUKPLAT_NATIVE_SYSCTX
__isr static inline __uptr uk_plat_native_tlsp_get(void)
{
	__uptr tlsp;

	__asm__ __volatile__("mv %0, tp" : "=r" (tlsp));
	return tlsp;
}

__isr static inline void uk_plat_native_tlsp_set(__uptr tlsp)
{
	__asm__ __volatile__("mv tp, %0" : : "r" (tlsp) : "memory");
}

__isr void uk_plat_native_sysctx_store(struct uk_plat_native_sysctx *sysctx);
__isr void uk_plat_native_sysctx_load(struct uk_plat_native_sysctx *sysctx);
#endif /* CONFIG_LIBUKPLAT_NATIVE_SYSCTX */

__isr static inline __u64
uk_plat_native_sysctx_get(const struct uk_plat_native_sysctx *sc, __sz offset)
{
	return *(__u64 *)((const char *)sc + offset);
}

__isr static inline void
uk_plat_native_sysctx_set(struct uk_plat_native_sysctx *sc, __sz offset,
			  __u64 val)
{
	*(__u64 *)((char *)sc + offset) = val;
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_SYSCTX_H__ */
