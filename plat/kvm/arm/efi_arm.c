#include <kvm/efi.h>
#include <uk/arch/paging.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/common/sections.h>

extern void clean_and_invalidate_dcache_range(unsigned long, unsigned long);
extern void _ukplat_entry(void *) __noreturn;
extern __paddr_t vector_table;
extern void start_mmu(void);

static __u8 __align(16) uk_efi_bootstack[__PAGE_SIZE];

/* We cannot set the system register directly */
#define uk_efi_set_sp_el1(sp)						\
	__asm__ __volatile__("mov sp, %0\n" ::"r" (sp))

void __noreturn uk_efi_jmp_to_kern(void)
{
	ukplat_lcpu_disable_irq();

	/* Invalidate the image from the data cache */
	clean_and_invalidate_dcache_range(__BASE_ADDR, __END);

	SYSREG_WRITE64(sctlr_el1, SCTLR_SET_BITS);
	SYSREG_WRITE64(contextidr_el1, 0);
	SYSREG_WRITE64(VBAR_EL1, &vector_table);
	SYSREG_WRITE64(spsr_el1, 0);
	SYSREG_WRITE64(elr_el1, 0);
	SYSREG_WRITE64(cntv_cval_el0, 0);
	/* EDKII's backtrace and dump register code is written in C, so
	 * it requires a stack. If SP_EL1 is corrupted before an exception
	 * is taken, they rely on SP_EL0 which is set to a backup stack.
	 * Thus, reset it to 0 to avoid any back reference to firmware.
	 */
	SYSREG_WRITE64(sp_el0, 0);
	SYSREG_WRITE64(tpidr_el0, 0);

	start_mmu();
	uk_efi_set_sp_el1(uk_efi_bootstack + __PAGE_SIZE);
	_ukplat_entry(ukplat_bootinfo_get());
}
