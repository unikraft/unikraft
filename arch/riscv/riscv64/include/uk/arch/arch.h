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

static inline __u8 uk_arch_ioreg_read8(const volatile __u8 *address)
{
	return uk_arch_riscv64_ioreg_read8(address);
}

static inline __u16 uk_arch_ioreg_read16(const volatile __u16 *address)
{
	return uk_arch_riscv64_ioreg_read16(address);
}

static inline __u32 uk_arch_ioreg_read32(const volatile __u32 *address)
{
	return uk_arch_riscv64_ioreg_read32(address);
}

static inline __u64 uk_arch_ioreg_read64(const volatile __u64 *address)
{
	return uk_arch_riscv64_ioreg_read64(address);
}

static inline void uk_arch_ioreg_write8(const volatile __u8 *address,
					__u8 value)
{
	uk_arch_riscv64_ioreg_write8(address, value);
}

static inline void uk_arch_ioreg_write16(const volatile __u16 *address,
					 __u16 value)
{
	uk_arch_riscv64_ioreg_write16(address, value);
}

static inline void uk_arch_ioreg_write32(const volatile __u32 *address,
					 __u32 value)
{
	uk_arch_riscv64_ioreg_write32(address, value);
}

static inline void uk_arch_ioreg_write64(const volatile __u64 *address,
					 __u64 value)
{
	uk_arch_riscv64_ioreg_write64(address, value);
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
