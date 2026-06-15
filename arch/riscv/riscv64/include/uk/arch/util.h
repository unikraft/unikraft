#ifndef __UK_ARCH_UTIL_H__
#define __UK_ARCH_UTIL_H__

#include <uk/arch/riscv64.h>
#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !__ASSEMBLY__
#include <uk/essentials.h>

static inline __u64 uk_arch_riscv64_read_sp(void)
{
	__u64 sp;

	__asm__ __volatile__("mv %0, sp" : "=&r"(sp));

	return sp;
}

static inline void uk_arch_riscv64_spinwait(void)
{
	__asm__ __volatile__("nop" ::: "memory");
}

static inline void uk_arch_riscv64_mb(void)
{
	__asm__ __volatile__("fence" ::: "memory");
}

static inline void uk_arch_riscv64_rmb(void)
{
	__asm__ __volatile__("fence ir, ir" ::: "memory");
}

static inline void uk_arch_riscv64_wmb(void)
{
	__asm__ __volatile__("fence ow, ow" ::: "memory");
}

static inline void __noreturn uk_arch_riscv64_jump_to(__u64 sp, __u64 entry)
{
	__asm__ __volatile__(
		"mv s0, zero\n"
		"mv ra, zero\n"
		"mv sp, %0\n"
		"jr %1\n"
		:
		: "r"(sp), "r"(entry)
		: "memory");

	__builtin_unreachable();
}

static inline void __noreturn
uk_arch_riscv64_jump_to_with_arg(__u64 sp, __u64 entry, __u64 arg)
{
	__asm__ __volatile__(
		"mv s0, zero\n"
		"mv ra, zero\n"
		"mv sp, %0\n"
		"mv a0, %2\n"
		"jr %1\n"
		:
		: "r"(sp), "r"(entry), "r"(arg)
		: "a0", "memory");

	__builtin_unreachable();
}

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_ARCH_UTIL_H__ */
