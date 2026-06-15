#ifndef __UK_ARCH_H__
#error "Do not include this header directly"
#endif

#include <uk/arch/util.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__

static inline void uk_arch_mb(void)
{
	uk_arch_riscv64_mb();
}

static inline void uk_arch_rmb(void)
{
	uk_arch_riscv64_rmb();
}

static inline void uk_arch_wmb(void)
{
	uk_arch_riscv64_wmb();
}

static inline __u64 uk_arch_read_sp(void)
{
	return uk_arch_riscv64_read_sp();
}

static inline void uk_arch_spinwait(void)
{
	uk_arch_riscv64_spinwait();
}

static inline void __noreturn uk_arch_jump_to(__u64 sp, __u64 entry)
{
	uk_arch_riscv64_jump_to(sp, entry);
}

static inline void __noreturn
uk_arch_jump_to_with_arg(__u64 sp, __u64 ip, __u64 arg)
{
	uk_arch_riscv64_jump_to_with_arg(sp, ip, arg);
}

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif
