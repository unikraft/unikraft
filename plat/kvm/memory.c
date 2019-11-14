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

#include <uk/plat/common/sections.h>
#include <sys/types.h>
#include <uk/plat/memory.h>
#include <uk/assert.h>
#include <kvm/config.h>

int ukplat_memregion_count(void)
{
	return (9
		+ ((_libkvmplat_cfg.initrd.len > 0) ? 1 : 0)
		+ ((_libkvmplat_cfg.heap2.len  > 0) ? 1 : 0));
}

int ukplat_memregion_get(int i, struct ukplat_memregion_desc *m)
{
	int ret;

	UK_ASSERT(m);

	switch (i) {
	case 0: /* text */
		m->base  = (void *) __TEXT;
		m->len   = (size_t) __ETEXT - (size_t) __TEXT;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE);
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "text";
#endif
		ret = 0;
		break;
	case 1: /* eh_frame */
		m->base  = (void *) __EH_FRAME_START;
		m->len   = (size_t) __EH_FRAME_END
				- (size_t) __EH_FRAME_START;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE);
		ret = 0;
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "eh_frame";
#endif
		break;
	case 2: /* eh_frame_hdr */
		m->base  = (void *) __EH_FRAME_HDR_START;
		m->len   = (size_t) __EH_FRAME_HDR_END
			   - (size_t) __EH_FRAME_HDR_START;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE);
		ret = 0;
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "eh_frame_hdr";
#endif
		break;
	case 3: /* rodata */
		m->base  = (void *) __RODATA;
		m->len   = (size_t) __ERODATA - (size_t) __RODATA;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE);
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "rodata";
#endif
		ret = 0;
		break;
	case 4: /* ctors */
		m->base  = (void *) __CTORS;
		m->len   = (size_t) __ECTORS - (size_t) __CTORS;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE);
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "ctors";
#endif
		ret = 0;
		break;
	case 5: /* data */
		m->base  = (void *) __DATA;
		m->len   = (size_t) __EDATA - (size_t) __DATA;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE
			    | UKPLAT_MEMRF_WRITABLE);
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "data";
#endif
		ret = 0;
		break;
	case 6: /* bss */
		m->base  = (void *) __BSS_START;
		m->len   = (size_t) __END - (size_t) __BSS_START;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE
			    | UKPLAT_MEMRF_WRITABLE);
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "bss";
#endif
		ret = 0;
		break;
	case 7: /* heap */
		m->base  = (void *) _libkvmplat_cfg.heap.start;
		m->len   = _libkvmplat_cfg.heap.len;
		m->flags = UKPLAT_MEMRF_ALLOCATABLE;
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "heap";
#endif
		ret = 0;
		break;
	case 8: /* stack */
		m->base  = (void *) _libkvmplat_cfg.bstack.start;
		m->len   = _libkvmplat_cfg.bstack.len;
		m->flags = (UKPLAT_MEMRF_RESERVED
			    | UKPLAT_MEMRF_READABLE
			    | UKPLAT_MEMRF_WRITABLE);
		ret = 0;
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "bstack";
#endif
		break;
	case 9: /* initrd */
		if (_libkvmplat_cfg.initrd.len) {
			m->base  = (void *) _libkvmplat_cfg.initrd.start;
			m->len   = _libkvmplat_cfg.initrd.len;
			m->flags = (UKPLAT_MEMRF_INITRD |
				    UKPLAT_MEMRF_WRITABLE);
#if CONFIG_UKPLAT_MEMRNAME
			m->name  = "initrd";
#endif
			ret = 0;
			break;
		}
		/* fall-through */
	case 10: /* heap2
		 *  NOTE: heap2 could only exist if initrd was there,
		 *  otherwise we fall through */
		if (_libkvmplat_cfg.initrd.len && _libkvmplat_cfg.heap2.len) {
			m->base  = (void *) _libkvmplat_cfg.heap2.start;
			m->len   = _libkvmplat_cfg.heap2.len;
			m->flags = UKPLAT_MEMRF_ALLOCATABLE;
#if CONFIG_UKPLAT_MEMRNAME
			m->name  = "heap";
#endif
			ret = 0;
			break;
		}
		/* fall-through */
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
