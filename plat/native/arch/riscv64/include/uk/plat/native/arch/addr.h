#ifndef __UK_PLAT_NATIVE_ARCH_ADDR_H__
#define __UK_PLAT_NATIVE_ARCH_ADDR_H__

#include <uk/asm/paging.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <uk/plat/native/arch/page.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_ADDR

#if !__ASSEMBLY__

#define UK_PLAT_NATIVE_VADDR_INV	__VADDR_INV
#define UK_PLAT_NATIVE_PADDR_INV	__PADDR_INV

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

#include <uk/assert.h>

#define UK_PLAT_NATIVE_DIRECTMAP_AREA_START				\
	RISCV64_VADDR_CANONICALIZE(0x4000000000UL)
#define UK_PLAT_NATIVE_DIRECTMAP_AREA_END				\
	RISCV64_VADDR_CANONICALIZE(0x7fffffffffUL)

static inline int uk_plat_native_vaddr_range_isvalid(__vaddr_t start, __sz len)
{
	__vaddr_t end = start + len - 1;

	UK_ASSERT(start <= end);

	if (RISCV64_VADDR_CANONICALIZE(start) != start)
		return 0;

	if (RISCV64_VADDR_CANONICALIZE(end) != end)
		return 0;

	return (((start ^ end) & (1UL << (RISCV64_VADDR_BITS - 1))) == 0);
}

static inline int uk_plat_native_vaddr_isvalid(__vaddr_t addr)
{
	return RISCV64_VADDR_CANONICALIZE(addr) == addr;
}

static inline int uk_plat_native_paddr_range_isvalid(__paddr_t start, __sz len)
{
	__paddr_t end = start + len - 1;

	UK_ASSERT(start <= end);
	return (start <= RISCV64_PADDR_MAX &&
		end <= RISCV64_PADDR_MAX);
}

static inline int uk_plat_native_paddr_isvalid(__paddr_t addr)
{
	return addr <= RISCV64_PADDR_MAX;
}
#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_ADDR */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_ADDR_H__ */
