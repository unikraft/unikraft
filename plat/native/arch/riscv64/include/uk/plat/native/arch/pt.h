#ifndef __UK_PLAT_NATIVE_ARCH_PT_H__
#define __UK_PLAT_NATIVE_ARCH_PT_H__

#include <uk/asm/paging.h>
#include <uk/asm/lcpu.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <uk/plat/native/arch/page.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_PT

#define UK_PLAT_NATIVE_PT_LEVELS		PT_LEVELS
#define UK_PLAT_NATIVE_PTES_PER_LEVEL		RISCV64_PT_PTES_PER_LEVEL
#define UK_PLAT_NATIVE_PT_LEVEL_SHIFT		RISCV64_PT_LEVEL_SHIFT

#define UK_PLAT_NATIVE_PT_Lx_PTES(lvl)		PT_Lx_PTES(lvl)
#define UK_PLAT_NATIVE_PT_Lx_IDX(vaddr, lvl)	PT_Lx_IDX(vaddr, lvl)
#define UK_PLAT_NATIVE_PT_Lx_PTE_PRESENT(pte, lvl)	PT_Lx_PTE_PRESENT(pte, lvl)
#define UK_PLAT_NATIVE_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)	\
	PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)
#define UK_PLAT_NATIVE_PT_Lx_PTE_INVALID(lvl)	PT_Lx_PTE_INVALID(lvl)

#if !__ASSEMBLY__

#if CONFIG_LIBUKPLAT_NATIVE_PAGING

static inline __paddr_t
UK_PLAT_NATIVE_PT_Lx_PTE_PADDR(__pte_t pte, unsigned int lvl)
{
	(void)lvl;
	return PT_Lx_PTE_PADDR(pte, lvl);
}

static inline __pte_t
UK_PLAT_NATIVE_PT_Lx_PTE_SET_PADDR(__pte_t pte, unsigned int lvl,
				   __paddr_t paddr)
{
	(void)lvl;
	return PT_Lx_PTE_SET_PADDR(pte, lvl, paddr);
}

static inline int
uk_plat_native_pte_read(__vaddr_t pt_vaddr, unsigned int lvl,
			unsigned int idx, __pte_t *pte)
{
	(void)lvl;
	UK_ASSERT(idx < UK_PLAT_NATIVE_PT_Lx_PTES(lvl));
	*pte = *((__pte_t *)pt_vaddr + idx);
	return 0;
}

static inline int
uk_plat_native_pte_write(__vaddr_t pt_vaddr, unsigned int lvl,
			 unsigned int idx, __pte_t pte)
{
	(void)lvl;
	UK_ASSERT(idx < UK_PLAT_NATIVE_PT_Lx_PTES(lvl));
	*((__pte_t *)pt_vaddr + idx) = pte;
	return 0;
}

static inline __pte_t
uk_plat_native_pte_create(__paddr_t paddr, unsigned long attr,
			  unsigned int level __unused, __pte_t tmpl,
			  unsigned int tmpl_level __unused)
{
	__pte_t pte;

	UK_ASSERT(UK_PLAT_NATIVE_PAGE_ALIGNED(paddr));

	pte = RISCV64_PADDR_TO_PTE_PPN(paddr);
	pte |= RISCV64_PTE_VALID | RISCV64_PTE_GLOBAL;

	if (attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_READ)
		pte |= RISCV64_PTE_READ;

	if (attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_WRITE)
		pte |= RISCV64_PTE_WRITE;

	if (attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_EXEC)
		pte |= RISCV64_PTE_EXEC;

	pte |= tmpl & (RISCV64_PTE_USER |
		       RISCV64_PTE_ACCESSED |
		       RISCV64_PTE_DIRTY |
		       RISCV64_PTE_RSW_MASK);

	return pte;
}

static inline __paddr_t uk_plat_native_pt_read_base(void)
{
	__pte_t satp;

	satp = _csr_read(UK_ARCH_RISCV64_CSR_SATP);
	return RISCV64_PPN_TO_PADDR(satp & UK_ARCH_RISCV64_SATP_PPN_MASK);
}

static inline int uk_plat_native_pt_write_base(__paddr_t pt_paddr)
{
	__pte_t satp = RISCV64_SET_SATP_MODE(UK_ARCH_RISCV64_SATP_MODE_SV39) |
		       RISCV64_SET_SATP_ASID(0) |
		       RISCV64_SET_SATP_PPN(RISCV64_PADDR_TO_PPN(pt_paddr));

	_csr_write(UK_ARCH_RISCV64_CSR_SATP, satp);
	__asm__ __volatile__("sfence.vma x0, x0" ::: "memory");
	return 0;
}

static inline __pte_t
uk_plat_native_pte_change_attr(__pte_t pte, unsigned long new_attr,
			       unsigned int level __unused)
{
	pte &= ~(RISCV64_PTE_RW | RISCV64_PTE_EXEC);

	if (new_attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_READ)
		pte |= RISCV64_PTE_READ;

	if (new_attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_WRITE)
		pte |= RISCV64_PTE_WRITE;

	if (new_attr & UK_PLAT_NATIVE_PAGE_ATTR_PROT_EXEC)
		pte |= RISCV64_PTE_EXEC;

	return pte;
}

static inline unsigned long
uk_plat_native_attr_from_pte(__pte_t pte, unsigned int level __unused)
{
	unsigned long attr = 0;

	if (pte & RISCV64_PTE_READ)
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_PROT_READ;

	if (pte & RISCV64_PTE_WRITE)
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_PROT_WRITE;

	if (pte & RISCV64_PTE_EXEC)
		attr |= UK_PLAT_NATIVE_PAGE_ATTR_PROT_EXEC;

	return attr;
}

static inline __pte_t
uk_plat_native_pt_pte_create(__paddr_t pt_paddr, unsigned int level __unused,
			     __pte_t tmpl, unsigned int tmpl_level __unused)
{
	__pte_t pt_pte;

	UK_ASSERT(UK_PLAT_NATIVE_PAGE_ALIGNED(pt_paddr));

	pt_pte = RISCV64_PADDR_TO_PTE_PPN(pt_paddr);
	pt_pte |= RISCV64_PTE_VALID_LINK;
	pt_pte |= tmpl & (RISCV64_PTE_USER |
			  RISCV64_PTE_ACCESSED |
			  RISCV64_PTE_DIRTY |
			  RISCV64_PTE_RSW_MASK);

	return pt_pte;
}

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGING */

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_PT */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_PT_H__ */
