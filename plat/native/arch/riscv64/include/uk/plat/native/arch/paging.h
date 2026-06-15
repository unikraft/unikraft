#ifndef __UK_PLAT_NATIVE_ARCH_PAGING_H__
#define __UK_PLAT_NATIVE_ARCH_PAGING_H__

#include <uk/assert.h>
#include <uk/asm/lcpu.h>

#if !__ASSEMBLY__

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

static inline int uk_plat_native_paging_init(void)
{
	if (!_csr_read(UK_ARCH_RISCV64_CSR_SATP))
		UK_CRASH("Sv39 is not supported by the MMU, crashing...\n");

	return 0;
}

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_NATIVE_ARCH_PAGING_H__ */
