#ifndef __UK_LCPU_H__
#error "Do not include this header directly"
#endif

#define UK_LCPU_RISCV64_SYSCTX_OFFSETOF_TP				\
	UK_PAL_RISCV64_SYSCTX_OFFSETOF_TP
#define UK_LCPU_RISCV64_SYSCTX_OFFSETOF_TLSP				\
	UK_PAL_SYSCTX_OFFSETOF_TLSP

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#define uk_lcpu_sysctx_get(_sc, _offset)				\
	(uk_pal_sysctx_get((struct uk_pal_sysctx *)(_sc),		\
			   UK_LCPU_RISCV64_SYSCTX_OFFSETOF_##_offset))

#define uk_lcpu_sysctx_set(_sc, _offset, _val)				\
	(uk_pal_sysctx_set((struct uk_pal_sysctx *)(_sc),		\
			   UK_LCPU_RISCV64_SYSCTX_OFFSETOF_##_offset, (_val)))

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
