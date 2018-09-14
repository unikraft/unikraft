/* SPDX-License-Identifier: ISC */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *
 * Copyright (c) 2018 Arm Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <libfdt.h>
#include <kvm/console.h>
#include <uk/assert.h>
#include <kvm-arm/mm.h>
#include <arm/cpu.h>
#include <uk/arch/limits.h>

void *_libkvmplat_pagetable;
void *_libkvmplat_heap_start;
void *_libkvmplat_stack_top;
void *_libkvmplat_mem_end;
void *_libkvmplat_dtb;

#define MAX_CMDLINE_SIZE 1024
static char cmdline[MAX_CMDLINE_SIZE];

smcc_psci_callfn_t smcc_psci_call;

extern void _libkvmplat_newstack(uint64_t stack_start,
			void (*tramp)(void *), void *arg);

static void _init_dtb(void *dtb_pointer)
{
	int ret;

	if ((ret = fdt_check_header(dtb_pointer)))
		UK_CRASH("Invalid DTB: %s\n", fdt_strerror(ret));

	_libkvmplat_dtb = dtb_pointer;
	uk_printd(DLVL_INFO, "Found device tree on: %p\n", dtb_pointer);
}

static void _dtb_get_psci_method(void)
{
	int fdtpsci, len;
	const char *fdtmethod;

	/*
	 * We just support PSCI-0.2 and PSCI-1.0, the PSCI-0.1 would not
	 * be supported.
	 */
	fdtpsci = fdt_node_offset_by_compatible(_libkvmplat_dtb,
						-1, "arm,psci-1.0");
	if (fdtpsci < 0)
		fdtpsci = fdt_node_offset_by_compatible(_libkvmplat_dtb,
							-1, "arm,psci-0.2");
	if (fdtpsci < 0) {
		uk_printd(DLVL_INFO, "No PSCI conduit found in DTB\n");
		goto enomethod;
	}

	fdtmethod = fdt_getprop(_libkvmplat_dtb, fdtpsci, "method", &len);
	if (!fdtmethod || (len <= 0)) {
		uk_printd(DLVL_INFO, "No PSCI method found\n");
		goto enomethod;
	}

	if (!strcmp(fdtmethod, "hvc"))
		smcc_psci_call = smcc_psci_hvc_call;
	else if (!strcmp(fdtmethod, "smc"))
		smcc_psci_call = smcc_psci_smc_call;
	else {
		uk_printd(DLVL_INFO,
		"Invalid PSCI conduit method: %s\n", fdtmethod);
		goto enomethod;
	}

	uk_printd(DLVL_INFO, "PSCI method: %s\n", fdtmethod);
	return;

enomethod:
	uk_printd(DLVL_INFO, "Support PSCI from PSCI-0.2\n");
	smcc_psci_call = NULL;
}

static void _init_dtb_mem(void)
{
	extern char _text[];
	extern char _end[];
	int fdt_mem, prop_len = 0, prop_min_len;
	int naddr, nsize;
	const uint64_t *regs;
	uint64_t mem_base, mem_size, max_addr;

	/* search for assigned VM memory in DTB */
	if (fdt_num_mem_rsv(_libkvmplat_dtb) != 0)
		uk_printd(DLVL_WARN, "Reserved memory is not supported\n");

	fdt_mem = fdt_node_offset_by_prop_value(_libkvmplat_dtb, -1,
						"device_type",
						"memory", sizeof("memory"));
	if (fdt_mem < 0) {
		uk_printd(DLVL_WARN, "No memory found in DTB\n");
		return;
	}

	naddr = fdt_address_cells(_libkvmplat_dtb, fdt_mem);
	if (naddr < 0 || naddr >= FDT_MAX_NCELLS)
		UK_CRASH("Could not find proper address cells!\n");

	nsize = fdt_size_cells(_libkvmplat_dtb, fdt_mem);
	if (nsize < 0 || nsize >= FDT_MAX_NCELLS)
		UK_CRASH("Could not find proper size cells!\n");

	/*
	 * QEMU will always provide us at least one bank of memory.
	 * unikraft will use the first bank for the time-being.
	 */
	regs = fdt_getprop(_libkvmplat_dtb, fdt_mem, "reg", &prop_len);

	/*
	 * The property must contain at least the start address
	 * and size, each of which is 8-bytes.
	 */
	prop_min_len = (int)sizeof(fdt32_t) * (naddr + nsize);
	if (regs == NULL || prop_len < prop_min_len)
		UK_CRASH("Bad 'reg' property: %p %d\n", regs, prop_len);

	/* If we have more than one memory bank, give a warning messasge */
	if (prop_len > prop_min_len)
		uk_printd(DLVL_WARN,
			"Currently, we support only one memory bank!\n");

	mem_base = fdt64_to_cpu(regs[0]);
	mem_size = fdt64_to_cpu(regs[1]);
	if (mem_base > (uint64_t)&_text)
		UK_CRASH("Fatal: Image outside of RAM\n");

	max_addr = mem_base + mem_size;
	_libkvmplat_pagetable =(void *) ALIGN_DOWN((size_t)&_end, __PAGE_SIZE);
	_libkvmplat_heap_start = _libkvmplat_pagetable + PAGE_TABLE_SIZE;
	_libkvmplat_mem_end = (void *) max_addr;

	/* AArch64 require stack be 16-bytes alignment by default */
	_libkvmplat_stack_top = (void *) ALIGN_UP(max_addr, __STACK_ALIGN_SIZE);
}

static void _dtb_get_cmdline(char *cmdline, size_t maxlen)
{
	int fdtchosen, len;
	const char *fdtcmdline;

	/* TODO: Proper error handling */
	fdtchosen = fdt_path_offset(_libkvmplat_dtb, "/chosen");
	if (!fdtchosen)
		goto enocmdl;
	fdtcmdline = fdt_getprop(_libkvmplat_dtb, fdtchosen, "bootargs", &len);
	if (!fdtcmdline || (len <= 0))
		goto enocmdl;

	strncpy(cmdline, fdtcmdline, MIN(maxlen, (unsigned int) len));
	/* ensure null termination */
	cmdline[((unsigned int) len - 1) <= (maxlen - 1) ?
		((unsigned int) len - 1) : (maxlen - 1)] = '\0';

	uk_printd(DLVL_INFO, "Command line: %s\n", cmdline);
	return;

enocmdl:
	uk_printd(DLVL_INFO, "No command line found\n");
	strcpy(cmdline, CONFIG_UK_NAME);
}

static void _libkvmplat_entry2(void *arg __attribute__((unused)))
{
       ukplat_entry_argp(NULL, (char *)cmdline, strlen(cmdline));
}

void _libkvmplat_start(void *dtb_pointer)
{
	_init_dtb(dtb_pointer);
	_libkvmplat_init_console();

	uk_printd(DLVL_INFO, "Entering from KVM (arm64)...\n");

	/* Get command line from DTB */

	_dtb_get_cmdline(cmdline, sizeof(cmdline));

	/* Get PSCI method from DTB */
	_dtb_get_psci_method();

	/* Initialize memory from DTB */
	_init_dtb_mem();

	uk_printd(DLVL_INFO, "pagetable start: %p\n", _libkvmplat_pagetable);
	uk_printd(DLVL_INFO, "     heap start: %p\n", _libkvmplat_heap_start);
	uk_printd(DLVL_INFO, "      stack top: %p\n", _libkvmplat_stack_top);

	/*
	 * Switch away from the bootstrap stack as early as possible.
	 */
	uk_printd(DLVL_INFO, "Switch from bootstrap stack to stack @%p\n",
				_libkvmplat_stack_top);

	_libkvmplat_newstack((uint64_t) _libkvmplat_stack_top,
				_libkvmplat_entry2, NULL);
}
