#ifndef __UK_PLAT_PAL_EXCEPT_H__
#error "Do not include this header directly"
#endif

#define UK_PAL_RISCV64_EXCEPT_ID_INVALID_OP				\
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_INVALID_OP
#define UK_PAL_RISCV64_EXCEPT_ID_DEBUG					\
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_DEBUG
#define UK_PAL_RISCV64_EXCEPT_ID_PAGE_FAULT				\
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_PAGE_FAULT
#define UK_PAL_RISCV64_EXCEPT_ID_BUS_ERROR				\
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_BUS_ERROR
#define UK_PAL_RISCV64_EXCEPT_ID_MATH					\
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_MATH
#define UK_PAL_RISCV64_EXCEPT_ID_SECURITY				\
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_SECURITY
#define UK_PAL_RISCV64_EXCEPT_ID_SYSCALL				\
	UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_SYSCALL

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uk_pal_except_err_ctx;

__isr static inline int
uk_pal_riscv64_except_err_ctx_get_eid(const struct uk_pal_except_err_ctx *ctx)
{
	return uk_plat_native_riscv64_except_err_ctx_get_eid(
		(const struct uk_plat_native_except_err_ctx *)ctx);
}

__isr static inline void
uk_pal_riscv64_except_err_ctx_set_eid(struct uk_pal_except_err_ctx *ctx,
				      int eid)
{
	uk_plat_native_riscv64_except_err_ctx_set_eid(
		(struct uk_plat_native_except_err_ctx *)ctx, eid);
}

__isr static inline __u64
uk_pal_riscv64_except_err_ctx_get_scause(const struct uk_pal_except_err_ctx *ctx)
{
	return uk_plat_native_riscv64_except_err_ctx_get_scause(
		(const struct uk_plat_native_except_err_ctx *)ctx);
}

__isr static inline void
uk_pal_riscv64_except_err_ctx_set_scause(struct uk_pal_except_err_ctx *ctx,
					 __u64 scause)
{
	uk_plat_native_riscv64_except_err_ctx_set_scause(
		(struct uk_plat_native_except_err_ctx *)ctx, scause);
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
