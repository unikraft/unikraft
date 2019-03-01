/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dan Williams
 *          Martin Lucina
 *          Ricardo Koller
 *          Felipe Huici <felipe.huici@neclab.eu>
 *          Florian Schmidt <florian.schmidt@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2017 NEC Europe Ltd., NEC Corporation
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

#include <string.h>
#include <sections.h>
#include <x86/cpu.h>
#include <x86/traps.h>
#include <kvm/console.h>
#include <kvm/intctrl.h>
#include <kvm-x86/multiboot.h>
#include <kvm-x86/multiboot_defs.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/plat/console.h>
#include <uk/assert.h>
#include <uk/essentials.h>

#define PLATFORM_MEM_START 0x100000
#define PLATFORM_MAX_MEM_ADDR 0x40000000

#define MAX_CMDLINE_SIZE 8192
static char cmdline[MAX_CMDLINE_SIZE];

void *_libkvmplat_heap_start;
void *_libkvmplat_stack_top;
void *_libkvmplat_mem_end;

extern void _libkvmplat_newstack(__u64 stack_start, void (*tramp)(void *),
				void *arg);

static inline void _mb_get_cmdline(struct multiboot_info *mi, char *cmdline,
				   size_t maxlen)
{
	size_t cmdline_len;
	char *mi_cmdline;

	if (mi->flags & MULTIBOOT_INFO_CMDLINE) {
		mi_cmdline = (char *)(__u64)mi->cmdline;
		cmdline_len = strlen(mi_cmdline);

		if (cmdline_len >= maxlen) {
			cmdline_len = maxlen - 1;
			uk_pr_info("Command line too long, truncated\n");
		}
		memcpy(cmdline, mi_cmdline, cmdline_len);

		/* ensure null termination */
		cmdline[cmdline_len <= (maxlen - 1) ? cmdline_len
			: (maxlen - 1)] = '\0';
	} else {
		uk_pr_info("No command line found\n");
		strcpy(cmdline, CONFIG_UK_NAME);
	}
}

static inline void _mb_init_mem(struct multiboot_info *mi)
{
	multiboot_memory_map_t *m;
	size_t offset, max_addr;

	/*
	 * Look for the first chunk of memory at PLATFORM_MEM_START.
	 */
	for (offset = 0; offset < mi->mmap_length;
	     offset += m->size + sizeof(m->size)) {
		m = (void *)(__uptr)(mi->mmap_addr + offset);
		if (m->addr == PLATFORM_MEM_START
		    && m->type == MULTIBOOT_MEMORY_AVAILABLE) {
			break;
		}
	}
	UK_ASSERT(offset < mi->mmap_length);

	/*
	 * Cap our memory size to PLATFORM_MAX_MEM_SIZE which boot.S defines
	 * page tables for.
	 */
	max_addr = m->addr + m->len;
	if (max_addr > PLATFORM_MAX_MEM_ADDR)
		max_addr = PLATFORM_MAX_MEM_ADDR;
	UK_ASSERT((size_t)__END <= max_addr);

	_libkvmplat_heap_start = (void *) ALIGN_UP((size_t)__END, __PAGE_SIZE);
	_libkvmplat_mem_end    = (void *) max_addr;
	_libkvmplat_stack_top  = (void *) (max_addr - __STACK_SIZE);
}

static void _libkvmplat_entry2(void *arg __attribute__((unused)))
{
	ukplat_entry_argp(NULL, cmdline, sizeof(cmdline));
}

void _libkvmplat_entry(void *arg)
{
	struct multiboot_info *mi = (struct multiboot_info *)arg;

	_init_cpufeatures();
	_libkvmplat_init_console();
	traps_init();
	intctrl_init();

	uk_pr_info("Entering from KVM (x86)...\n");
	uk_pr_info("     multiboot: %p\n", mi);

	/*
	 * The multiboot structures may be anywhere in memory, so take a copy of
	 * everything necessary before we initialise memory allocation.
	 */
	_mb_get_cmdline(mi, cmdline, sizeof(cmdline));
	_mb_init_mem(mi);

	uk_pr_info("    heap start: %p\n", _libkvmplat_heap_start);
	uk_pr_info("     stack top: %p\n", _libkvmplat_stack_top);

	/*
	 * Switch away from the bootstrap stack as early as possible.
	 */
	uk_pr_info("Switch from bootstrap stack to stack @%p\n",
		   _libkvmplat_mem_end);
	_libkvmplat_newstack((__u64) _libkvmplat_mem_end,
				_libkvmplat_entry2, 0);
}
