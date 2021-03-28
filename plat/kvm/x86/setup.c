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
#include <uk/plat/common/sections.h>
#include <x86/cpu.h>
#include <x86/traps.h>
#include <kvm/config.h>
#include <kvm/console.h>
#include <kvm/intctrl.h>
#include <kvm-x86/multiboot2.h>
#include <kvm-x86/multiboot2_defs.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/plat/console.h>
#include <uk/assert.h>
#include <uk/essentials.h>

#define PLATFORM_MEM_START 0x100000
#define PLATFORM_MAX_MEM_ADDR 0x40000000

#define MAX_CMDLINE_SIZE 8192
static char cmdline[MAX_CMDLINE_SIZE];
static char have_cmdline, have_initrd;

struct kvmplat_config _libkvmplat_cfg = {0};

extern void _libkvmplat_newstack(uintptr_t stack_start, void (*tramp)(void *),
				 void *arg);

static inline void _mb_get_cmdline(void)
{
	if (!have_cmdline) {
		/* Use image name as cmdline to provide argv[0] */
		uk_pr_debug("No command line present\n");
		strncpy(cmdline, CONFIG_UK_NAME, sizeof(cmdline));
	}

	/* ensure null termination */
	cmdline[(sizeof(cmdline) - 1)] = '\0';
}

static inline void _mb_get_acpi(struct multiboot_info *mi)
{
	return;
}

static inline void _mb_init_mem(struct multiboot_tag *tag)
{
	multiboot_memory_map_t *m;
	size_t offset, max_addr;

	/*
	 * Look for the first chunk of memory at PLATFORM_MEM_START.
	 */

	for (m = ((struct multiboot_tag_mmap *)tag)->entries;
	     (multiboot_uint8_t *)m < (multiboot_uint8_t *)tag + tag->size;
	     m = (multiboot_memory_map_t *)((unsigned long)m
					    + ((struct multiboot_tag_mmap *)tag)
						  ->entry_size)) {
		if (m->addr == PLATFORM_MEM_START
		    && m->type == MULTIBOOT_MEMORY_AVAILABLE)
			break;
	}

	/*
	 * Cap our memory size to PLATFORM_MAX_MEM_SIZE which boot.S defines
	 * page tables for.
	 */
	max_addr = m->addr + m->len;
	if (max_addr > PLATFORM_MAX_MEM_ADDR)
		max_addr = PLATFORM_MAX_MEM_ADDR;
	UK_ASSERT((size_t)__END <= max_addr);

	/*
	 * Reserve space for boot stack at the end of found memory
	 */
	if ((max_addr - m->addr) < __STACK_SIZE)
		UK_CRASH("Not enough memory to allocate boot stack\n");

	_libkvmplat_cfg.heap.start = ALIGN_UP((uintptr_t)__END, __PAGE_SIZE);
	_libkvmplat_cfg.heap.end = (uintptr_t)max_addr - __STACK_SIZE;
	_libkvmplat_cfg.heap.len =
	    _libkvmplat_cfg.heap.end - _libkvmplat_cfg.heap.start;
	_libkvmplat_cfg.bstack.start = _libkvmplat_cfg.heap.end;
	_libkvmplat_cfg.bstack.end = max_addr;
	_libkvmplat_cfg.bstack.len = __STACK_SIZE;
}

static inline void _mb_init_initrd(struct multiboot_tag_module *mod1)
{
	uintptr_t heap0_start, heap0_end;
	uintptr_t heap1_start, heap1_end;
	size_t heap0_len, heap1_len;

	UK_ASSERT(mod1->mod_end >= mod1->mod_start);

	if (mod1->mod_end == mod1->mod_start) {
		uk_pr_debug("Ignoring empty initrd\n");
		goto no_initrd;
	}

	_libkvmplat_cfg.initrd.start = (uintptr_t)mod1->mod_start;
	_libkvmplat_cfg.initrd.end = (uintptr_t)mod1->mod_end;
	_libkvmplat_cfg.initrd.len = (size_t)(mod1->mod_end - mod1->mod_start);

	/*
	 * Check if initrd is part of heap
	 * In such a case, we figure out the remaining pieces as heap
	 */
	if (_libkvmplat_cfg.heap.len == 0) {
		/* We do not have a heap */
		goto out;
	}
	heap0_start = 0;
	heap0_end = 0;
	heap1_start = 0;
	heap1_end = 0;
	if (RANGE_OVERLAP(_libkvmplat_cfg.heap.start, _libkvmplat_cfg.heap.len,
			  _libkvmplat_cfg.initrd.start,
			  _libkvmplat_cfg.initrd.len)) {
		if (IN_RANGE(_libkvmplat_cfg.initrd.start,
			     _libkvmplat_cfg.heap.start,
			     _libkvmplat_cfg.heap.len)) {
			/* Start of initrd within heap range;
			 * Use the prepending left piece as heap
			 */
			heap0_start = _libkvmplat_cfg.heap.start;
			heap0_end = ALIGN_DOWN(_libkvmplat_cfg.initrd.start,
					       __PAGE_SIZE);
		}
		if (IN_RANGE(_libkvmplat_cfg.initrd.start,

			     _libkvmplat_cfg.heap.start,
			     _libkvmplat_cfg.heap.len)) {
			/* End of initrd within heap range;
			 * Use the remaining left piece as heap
			 */
			heap1_start =
			    ALIGN_UP(_libkvmplat_cfg.initrd.end, __PAGE_SIZE);
			heap1_end = _libkvmplat_cfg.heap.end;
		}
	} else {
		/* Initrd is not overlapping with heap */
		heap0_start = _libkvmplat_cfg.heap.start;
		heap0_end = _libkvmplat_cfg.heap.end;
	}
	heap0_len = heap0_end - heap0_start;
	heap1_len = heap1_end - heap1_start;

	/*
	 * Update heap regions
	 * We make sure that in we start filling left heap pieces at
	 * `_libkvmplat_cfg.heap`. Any additional piece will then be
	 * placed to `_libkvmplat_cfg.heap2`.
	 */
	if (heap0_len == 0) {
		/* Heap piece 0 is empty, use piece 1 as only */
		if (heap1_len != 0) {
			_libkvmplat_cfg.heap.start = heap1_start;
			_libkvmplat_cfg.heap.end = heap1_end;
			_libkvmplat_cfg.heap.len = heap1_len;
		} else {
			_libkvmplat_cfg.heap.start = 0;
			_libkvmplat_cfg.heap.end = 0;
			_libkvmplat_cfg.heap.len = 0;
		}
		_libkvmplat_cfg.heap2.start = 0;
		_libkvmplat_cfg.heap2.end = 0;
		_libkvmplat_cfg.heap2.len = 0;
	} else {
		/* Heap piece 0 has memory */
		_libkvmplat_cfg.heap.start = heap0_start;
		_libkvmplat_cfg.heap.end = heap0_end;
		_libkvmplat_cfg.heap.len = heap0_len;
		if (heap1_len != 0) {
			_libkvmplat_cfg.heap2.start = heap1_start;
			_libkvmplat_cfg.heap2.end = heap1_end;
			_libkvmplat_cfg.heap2.len = heap1_len;
		} else {
			_libkvmplat_cfg.heap2.start = 0;
			_libkvmplat_cfg.heap2.end = 0;
			_libkvmplat_cfg.heap2.len = 0;
		}
	}

	/*
	 * Double-check that initrd is not overlapping with previously allocated
	 * boot stack. We crash in such a case because we assume that multiboot
	 * places the initrd close to the beginning of the heap region. One need
	 * to assign just more memory in order to avoid this crash.
	 */
	if (RANGE_OVERLAP(_libkvmplat_cfg.heap.start, _libkvmplat_cfg.heap.len,
			  _libkvmplat_cfg.initrd.start,
			  _libkvmplat_cfg.initrd.len))
		UK_CRASH("Not enough space at end of memory for boot stack\n");
out:
	return;

no_initrd:
	_libkvmplat_cfg.initrd.start = 0;
	_libkvmplat_cfg.initrd.end = 0;
	_libkvmplat_cfg.initrd.len = 0;
	_libkvmplat_cfg.heap2.start = 0;
	_libkvmplat_cfg.heap2.end = 0;
	_libkvmplat_cfg.heap2.len = 0;
	return;
}

static void _libkvmplat_entry2(void *arg __attribute__((unused)))
{
	ukplat_entry_argp(NULL, cmdline, sizeof(cmdline));
}

static inline void _mb_parse_tags(struct multiboot_tag *tag)
{
	char *mi_cmdline;

	for (; tag->type != MULTIBOOT_TAG_TYPE_END;
	     tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag
					    + ((tag->size + 7) & ~7))) {
		uk_pr_debug("Tag 0x%x, Size 0x%x\n", tag->type, tag->size);
		switch (tag->type) {
		case MULTIBOOT_TAG_TYPE_CMDLINE:
			uk_pr_debug(
			    "Command line = %s\n",
			    ((struct multiboot_tag_string *)tag)->string);
			mi_cmdline =
			    ((struct multiboot_tag_string *)tag)->string;

			if (strlen(mi_cmdline) > sizeof(cmdline) - 1)
				uk_pr_err("Command line too long, truncated\n");
			strncpy(cmdline, mi_cmdline, sizeof(cmdline));
			have_cmdline = 1;
			break;
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
			uk_pr_debug(
			    "Boot loader name = %s\n",
			    ((struct multiboot_tag_string *)tag)->string);
			break;
		case MULTIBOOT_TAG_TYPE_MODULE:
			uk_pr_debug(
			    "Module at 0x%x-0x%x. Command line %s\n",
			    ((struct multiboot_tag_module *)tag)->mod_start,
			    ((struct multiboot_tag_module *)tag)->mod_end,
			    ((struct multiboot_tag_module *)tag)->cmdline);
			if (!have_initrd)
				_mb_init_initrd(
				    (struct multiboot_tag_module *)tag);
			break;
		case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
			uk_pr_debug("mem_lower = %uKB, mem_upper = %uKB\n",
				    ((struct multiboot_tag_basic_meminfo *)tag)
					->mem_lower,
				    ((struct multiboot_tag_basic_meminfo *)tag)
					->mem_upper);
			break;
		case MULTIBOOT_TAG_TYPE_BOOTDEV:
			uk_pr_debug(
			    "Boot device 0x%x,%u,%u\n",
			    ((struct multiboot_tag_bootdev *)tag)->biosdev,
			    ((struct multiboot_tag_bootdev *)tag)->slice,
			    ((struct multiboot_tag_bootdev *)tag)->part);
			break;
		case MULTIBOOT_TAG_TYPE_MMAP: {
			multiboot_memory_map_t *mmap;

			uk_pr_debug("mmap\n");

			for (mmap = ((struct multiboot_tag_mmap *)tag)->entries;
			     (multiboot_uint8_t *)mmap
			     < (multiboot_uint8_t *)tag + tag->size;
			     mmap = (multiboot_memory_map_t
					 *)((unsigned long)mmap
					    + ((struct multiboot_tag_mmap *)tag)
						  ->entry_size))
				uk_pr_debug(
				    " base_addr = 0x%x%x,"
				    " length = 0x%x%x, type = 0x%x\n",
				    (unsigned int)(mmap->addr >> 32),
				    (unsigned int)(mmap->addr & 0xffffffff),
				    (unsigned int)(mmap->len >> 32),
				    (unsigned int)(mmap->len & 0xffffffff),
				    (unsigned int)mmap->type);

			_mb_init_mem(tag);
		} break;
		case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
			multiboot_uint32_t color;
			unsigned int i;
			struct multiboot_tag_framebuffer *tagfb =
			    (struct multiboot_tag_framebuffer *)tag;
			void *fb = (void *)(unsigned long)
				       tagfb->common.framebuffer_addr;

			switch (tagfb->common.framebuffer_type) {
			case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED: {
				unsigned int best_distance, distance;
				struct multiboot_color *palette;

				palette = tagfb->framebuffer_palette;

				color = 0;
				best_distance = 4 * 256 * 256;

				for (i = 0;
				     i < tagfb->framebuffer_palette_num_colors;
				     i++) {
					distance =
					    (0xff - palette[i].blue)
						* (0xff - palette[i].blue)
					    + palette[i].red * palette[i].red
					    + palette[i].green
						  * palette[i].green;
					if (distance < best_distance) {
						color = i;
						best_distance = distance;
					}
				}
			} break;

			case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
				color =
				    ((1 << tagfb->framebuffer_blue_mask_size)
				     - 1)
				    << tagfb->framebuffer_blue_field_position;
				break;

			case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
				color = '\\' | 0x0100;
				break;

			default:
				color = 0xffffffff;
				break;
			}

			for (i = 0; i < tagfb->common.framebuffer_width
				    && i < tagfb->common.framebuffer_height;
			     i++) {
				switch (tagfb->common.framebuffer_bpp) {
				case 8: {
					multiboot_uint8_t *pixel =
					    fb
					    + tagfb->common.framebuffer_pitch
						  * i
					    + i;
					*pixel = color;
				} break;
				case 15:
				case 16: {
					multiboot_uint16_t *pixel =
					    fb
					    + tagfb->common.framebuffer_pitch
						  * i
					    + 2 * i;
					*pixel = color;
				} break;
				case 24: {
					multiboot_uint32_t *pixel =
					    fb
					    + tagfb->common.framebuffer_pitch
						  * i
					    + 3 * i;
					*pixel = (color & 0xffffff)
						 | (*pixel & 0xff000000);
				} break;

				case 32: {
					multiboot_uint32_t *pixel =
					    fb
					    + tagfb->common.framebuffer_pitch
						  * i
					    + 4 * i;
					*pixel = color;
				} break;
				}
			}
			break;
		}
		}
	}
}

void _libkvmplat_entry(void *arg)
{
	// struct multiboot_info *mi = (struct multiboot_info *)arg;
	struct multiboot_tag *tag;
	unsigned long mb = *(unsigned long *)arg;
	unsigned int size;

	_init_cpufeatures();
	_libkvmplat_init_console();
	traps_init();
	intctrl_init();

	uk_pr_info("Entering from KVM (x86)...\n");
	uk_pr_info("     multiboot: 0x%x\n", mb);

	if (mb & 7) {
		uk_pr_info("Unaligned mbi: 0x%x\n", mb);
		return;
	}

	size = *(unsigned int *)mb;
	uk_pr_info("Announced mbi size 0x%x\n", size);

	tag = (struct multiboot_tag *)(mb + 8);
	_mb_parse_tags(tag);

	/*
	 * The multiboot structures may be anywhere in memory, so take a copy of
	 * everything necessary before we initialise memory allocation.
	 */

	if (_libkvmplat_cfg.initrd.len)
		uk_pr_info("        initrd: %p\n",
			   (void *)_libkvmplat_cfg.initrd.start);
	uk_pr_info("    heap start: %p\n", (void *)_libkvmplat_cfg.heap.start);
	if (_libkvmplat_cfg.heap2.len)
		uk_pr_info(" heap start (2): %p\n",
			   (void *)_libkvmplat_cfg.heap2.start);
	uk_pr_info("     stack top: %p\n",
		   (void *)_libkvmplat_cfg.bstack.start);

#ifdef CONFIG_HAVE_SYSCALL
	_init_syscall();
#endif /* CONFIG_HAVE_SYSCALL */

#if CONFIG_HAVE_X86PKU
	_check_ospke();
#endif /* CONFIG_HAVE_X86PKU */

	/*
	 * Switch away from the bootstrap stack as early as possible.
	 */
	uk_pr_info("Switch from bootstrap stack to stack @%p\n",
		   (void *)_libkvmplat_cfg.bstack.end);
	_libkvmplat_newstack(_libkvmplat_cfg.bstack.end, _libkvmplat_entry2, 0);
}
