/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2015, IBM
 *           (c) 2017, NEC Europe Ltd.
 * Author(s): Dan Williams <djwillia@us.ibm.com>
 *            Simon Kuenzer <simon.kuenzer@neclab.eu>
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

#include <sys/types.h>
#include <uk/plat/memory.h>
#include <uk/assert.h>

/*
 * Provided by setup.c
 */
extern void *_libkvmplat_heap_start;
extern void *_libkvmplat_stack_top;
extern void *_libkvmplat_mem_end;

int ukplat_memregion_count(void)
{
	return 7;
}

int ukplat_memregion_get(int i, struct ukplat_memregion_desc *m)
{
	extern char _text, _etext, _data, _edata, _rodata, _erodata,
		    _ctors, _ectors, __bss_start, _end;
	int ret;

	UK_ASSERT(m);

	switch (i) {
	case 0: /* text */
		m->base  = &_text;
		m->len   = (size_t) &_etext - (size_t) &_text;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE);
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "text";
#endif
		ret = 0;
		break;
	case 1: /* rodata */
		m->base  = &_rodata;
		m->len   = (size_t) &_erodata - (size_t) &_rodata;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE);
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "rodata";
#endif
		ret = 0;
		break;
	case 2: /* ctors */
		m->base  = &_ctors;
		m->len   = (size_t) &_ectors - (size_t) &_ctors;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE);
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "ctors";
#endif
		ret = 0;
		break;
	case 3: /* data */
		m->base  = &_data;
		m->len   = (size_t) &_edata - (size_t) &_data;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE
			    | UKPLAT_MEMRF_WRITABLE);
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "data";
#endif
		ret = 0;
		break;
	case 4: /* bss */
		m->base  = &__bss_start;
		m->len   = (size_t) &_end - (size_t) &__bss_start;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE
			    | UKPLAT_MEMRF_WRITABLE);
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "bss";
#endif
		ret = 0;
		break;
	case 5: /* heap */
		m->base  = _libkvmplat_heap_start;
		m->len   = (size_t) _libkvmplat_stack_top
			   - (size_t) _libkvmplat_heap_start;
		m->flags = UKPLAT_MEMRF_ALLOCATABLE;
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "heap";
#endif
		ret = 0;
		break;
	case 6: /* stack */
		m->base  = _libkvmplat_stack_top;
		m->len   = (size_t) _libkvmplat_mem_end
			   - (size_t) _libkvmplat_stack_top;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE
			    | UKPLAT_MEMRF_WRITABLE);
		ret = 0;
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "bstack";
#endif
		break;
	default:
		m->base  = __NULL;
		m->len   = 0;
		m->flags = 0x0;
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = __NULL;
#endif
		ret = -1;
		break;
	}

	return ret;
}

int _ukplat_mem_mappings_init(void)
{
	return 0;
}
