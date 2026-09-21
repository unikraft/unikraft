/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/* Ported from Mini-OS */

#include <common/pt.h>
#include <xen-arm/os.h>
#include <xen-arm/mm.h>
#include <libfdt.h>
#include <uk/arch/util.h>
#include <uk/print.h>
#include <uk/assert.h>

#if defined(__aarch64__)
extern char stack[];
#endif

paddr_t physical_address_offset;

static void build_pagetable(unsigned long start_pfn, unsigned long max_pfn)
{
	unsigned int level = UK_PAL_PAGE_LARGE_LEVEL;
	__sz pgsize = UK_PAL_PAGE_Lx_SIZE(level);
	paddr_t start_paddr;
	__sz len;
	int rc;

	start_paddr = PFN_PHYS(start_pfn);
	len = PFN_PHYS(max_pfn);

	UK_ASSERT(UK_PAL_PAGE_Lx_ALIGNED(start_paddr, level));

	rc = xenplat_pt_map((__vaddr_t)to_virt(start_paddr), start_paddr,
			    (len + pgsize - 1) / pgsize, level,
			    XENPLAT_PT_ATTR_MEM);
	if (unlikely(rc))
		UK_CRASH("Could not map RAM: %d\n", rc);
}

void arch_mm_prepare(unsigned long *start_pfn_p, unsigned long *max_pfn_p)
{
	int memory;
	int prop_len = 0;
	unsigned long end;
	paddr_t mem_base;
	uint64_t mem_size;
	uint64_t heap_len;
	const uint64_t *regs;

	uk_pr_debug("    _text: %p(VA)\n", &_text);
	uk_pr_debug("    _etext: %p(VA)\n", &_etext);
	uk_pr_debug("    _erodata: %p(VA)\n", &_erodata);
	uk_pr_debug("    _edata: %p(VA)\n", &_edata);
	uk_pr_debug("    stack start: %p(VA)\n", stack);
	uk_pr_debug("    _end: %p(VA)\n", &_end);

	if (fdt_num_mem_rsv(HYPERVISOR_dtb) != 0)
		uk_pr_warn("WARNING: reserved memory not supported!\n");

	memory = fdt_node_offset_by_prop_value(HYPERVISOR_dtb, -1,
			 "device_type", "memory", sizeof("memory"));
	if (memory < 0) {
		uk_pr_warn("No memory found in FDT!\n");
		BUG();
	}

	/*
	 * Xen will always provide us at least one bank of memory.
	 * unikraft will use the first bank for the time-being.
	 */
	regs = fdt_getprop(HYPERVISOR_dtb, memory, "reg", &prop_len);

	/*
	 * The property must contain at least the start address
	 * and size, each of which is 8-bytes.
	 */
	if (regs == NULL || prop_len < 16)
		UK_CRASH("Bad 'reg' property: %p %d\n", regs, prop_len);

	end = (unsigned long) &_end;
	mem_base = fdt64_ld(regs);
	mem_size = fdt64_ld(regs + 1);

	uk_pr_debug("Found memory at 0x%llx (len 0x%llx)\n",
		(unsigned long long) mem_base, (unsigned long long) mem_size);

	build_pagetable(PHYS_PFN(mem_base), PHYS_PFN(mem_size));

	if (to_virt(mem_base) > (void *)__TEXT)
		UK_CRASH("Fatal: Image outside of RAM\n");

	*start_pfn_p = PFN_UP(to_phys(end));
	heap_len = mem_size - (PFN_PHYS(*start_pfn_p) - mem_base);
	*max_pfn_p = *start_pfn_p + PFN_DOWN(heap_len);
	uk_pr_debug("Using pages %lu to %lu as free space for heap.\n",
				*start_pfn_p, *max_pfn_p);
	uk_pr_info("    heap start: %p\n",
		   to_virt(*start_pfn_p << __PAGE_SHIFT));
}
