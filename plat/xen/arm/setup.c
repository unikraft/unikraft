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

#include <string.h>
#include <uk/plat/common/sections.h>
#include <xen-arm/os.h>
#include <xen-arm/mm.h>
#include <xen/xen.h>
#include <xen/memory.h>
#include <xen/hvm/params.h>
#include <common/events.h>
#include <libfdt.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/memory.h>
#include <uk/config.h>
#include <uk/assert.h>
#include <uk/essentials.h>

/*
 * Shared page for communicating with the hypervisor.
 * Events flags go here, for example.
 */
shared_info_t *HYPERVISOR_shared_info;

/*
 * Device tree
 */
void *HYPERVISOR_dtb;

/*
 * Physical address offset
 */
uint32_t _libxenplat_paddr_offset;

/*
 * Memory region description
 */
#define UKPLAT_MEMRD_MAX_ENTRIES 7
unsigned int _libxenplat_mrd_num;
struct ukplat_memregion_desc _libxenplat_mrd[UKPLAT_MEMRD_MAX_ENTRIES];

/*
 * Command line handling
 */
#define MAX_CMDLINE_SIZE 1024
static char cmdline[MAX_CMDLINE_SIZE];


static inline void _init_dtb(void *dtb_pointer)
{
	int ret;

	if ((ret = fdt_check_header(dtb_pointer)))
		UK_CRASH("Invalid DTB: %s\n", fdt_strerror(ret));
	HYPERVISOR_dtb = dtb_pointer;
}

static inline void _dtb_get_cmdline(char *cmdline, size_t maxlen)
{
	int fdtchosen, len;
	const char *fdtcmdline;

	/* TODO: Proper error handling */
	fdtchosen = fdt_path_offset(HYPERVISOR_dtb, "/chosen");
	if (!fdtchosen)
		goto enocmdl;
	fdtcmdline = fdt_getprop(HYPERVISOR_dtb, fdtchosen, "bootargs", &len);
	if (!fdtcmdline || (len <= 0))
		goto enocmdl;

	strncpy(cmdline, fdtcmdline, MIN(maxlen, (unsigned int) len));
	/* ensure null termination */
	cmdline[((unsigned int) len - 1) <= (maxlen - 1) ?
		((unsigned int) len - 1) : (maxlen - 1)] = '\0';

	return;

enocmdl:
	uk_pr_info("No command line found\n");
	strcpy(cmdline, CONFIG_UK_NAME);
}

static inline void _dtb_init_mem(uint32_t physical_offset)
{
	int memory;
	int prop_len = 0;
	const uint64_t *regs;
	uintptr_t end;
	uint64_t mem_base;
	uint64_t mem_size;
	uint64_t heap_len;
	size_t fdt_size;
	void *new_dtb;
	paddr_t start_pfn_p;
	paddr_t max_pfn_p;

	/* init physical address offset gathered by entry32.S */
	_libxenplat_paddr_offset = physical_offset;

	/* search for assigned VM memory in DTB */
	if (fdt_num_mem_rsv(HYPERVISOR_dtb) != 0)
		uk_pr_warn("Reserved memory is not supported\n");

	memory = fdt_node_offset_by_prop_value(HYPERVISOR_dtb, -1,
					       "device_type",
					       "memory", sizeof("memory"));
	if (memory < 0) {
		uk_pr_warn("No memory found in DTB\n");
		return;
	}

	/* Xen will always provide us at least one bank of memory.
	 * unikraft will use the first bank for the time-being.
	 */
	regs = fdt_getprop(HYPERVISOR_dtb, memory, "reg", &prop_len);
	/* The property must contain at least the start address
	 * and size, each of which is 8-bytes.
	 */
	if (regs == NULL && prop_len < 16)
		UK_CRASH("Bad 'reg' property: %p %d\n", regs, prop_len);

	end = (uintptr_t) __END;
	mem_base = fdt64_to_cpu(regs[0]);
	mem_size = fdt64_to_cpu(regs[1]);
	if (to_virt(mem_base) > (void *)__TEXT)
		UK_CRASH("Fatal: Image outside of RAM\n");

	start_pfn_p = PFN_UP(to_phys(end));
	heap_len = mem_size - (PFN_PHYS(start_pfn_p) - mem_base);
	max_pfn_p = start_pfn_p + PFN_DOWN(heap_len);
	uk_pr_info("    heap start: %p\n",
		   to_virt(start_pfn_p << __PAGE_SHIFT));

	/* The device tree is probably in memory that we're about to hand over
	 * to the page allocator, so move it to the end and reserve that space.
	 */
	fdt_size = fdt_totalsize(HYPERVISOR_dtb);
	new_dtb = to_virt(((max_pfn_p << __PAGE_SHIFT) - fdt_size)
			  & __PAGE_MASK);
	if (new_dtb != HYPERVISOR_dtb)
		memmove(new_dtb, HYPERVISOR_dtb, fdt_size);
	HYPERVISOR_dtb = new_dtb;
	max_pfn_p = to_phys(new_dtb) >> __PAGE_SHIFT;

	/* Fill out mrd array
	 */
	/* heap */
	_libxenplat_mrd[0].base  = to_virt(start_pfn_p << __PAGE_SHIFT);
	_libxenplat_mrd[0].len   = (size_t) to_virt(max_pfn_p << __PAGE_SHIFT)
		- (size_t) to_virt(start_pfn_p << __PAGE_SHIFT);
	_libxenplat_mrd[0].flags = (UKPLAT_MEMRF_ALLOCATABLE);
#if CONFIG_UKPLAT_MEMRNAME
	_libxenplat_mrd[0].name  = "heap";
#endif
	/* dtb */
	_libxenplat_mrd[1].base  = HYPERVISOR_dtb;
	_libxenplat_mrd[1].len   = fdt_size;
	_libxenplat_mrd[1].flags = (UKPLAT_MEMRF_RESERVED
				    | UKPLAT_MEMRF_READABLE);
#if CONFIG_UKPLAT_MEMRNAME
	_libxenplat_mrd[1].name  = "dtb";
#endif
	_libxenplat_mrd_num = 2;
}

void _libxenplat_armentry(void *dtb_pointer,
			  uint32_t physical_offset) __noreturn;

void _libxenplat_armentry(void *dtb_pointer, uint32_t physical_offset)
{
	uk_pr_info("Entering from Xen (arm)...\n");

	/* Zero'ing out the bss section */
	/*
	 * TODO: It probably makes sense to move this to the early
	 * platform entry assembly code.
	 */
	memset(__bss_start, 0, _end - __bss_start);

	_init_dtb(dtb_pointer);
	_dtb_init_mem(physical_offset); /* relocates dtb */
	uk_pr_info("           dtb: %p\n", HYPERVISOR_dtb);

	/* Set up events. */
	//init_events();

	/* ENABLE EVENT DELIVERY. This is disabled at start of day. */
	// TODO //

	_dtb_get_cmdline(cmdline, sizeof(cmdline));

	ukplat_entry_argp(CONFIG_UK_NAME, cmdline, sizeof(cmdline));
}

void _libxenplat_armpanic(int *saved_regs) __noreturn;

void _libxenplat_armpanic(int *saved_regs __unused)
{
	/* TODO: dump registers */
	ukplat_crash();
}
