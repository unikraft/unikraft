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

#include <string.h>
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

int ukplat_memregion_get(int i, struct ukplat_memregion_desc **m)
{
	static struct ukplat_memregion_desc mrd[11];

	UK_ASSERT(m);

	switch (i) {
	case 0: /* text */
		mrd[i].pbase = __TEXT;
		mrd[i].vbase = __TEXT;
		mrd[i].len   = __ETEXT - __TEXT;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_EXECUTE;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "text", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 1: /* eh_frame */
		mrd[i].pbase = __EH_FRAME_START;
		mrd[i].vbase = __EH_FRAME_START;
		mrd[i].len   = __EH_FRAME_END - __EH_FRAME_START;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "eh_frame", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 2: /* eh_frame_hdr */
		mrd[i].pbase = __EH_FRAME_HDR_START;
		mrd[i].vbase = __EH_FRAME_HDR_START;
		mrd[i].len   = __EH_FRAME_HDR_END - __EH_FRAME_HDR_START;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "eh_frame_hdr", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
		break;
	case 3: /* rodata */
		mrd[i].pbase = __RODATA;
		mrd[i].vbase = __RODATA;
		mrd[i].len   = __ERODATA - __RODATA;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "rodata", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 4: /* ctors */
		mrd[i].pbase = __CTORS;
		mrd[i].vbase = __CTORS;
		mrd[i].len   = __ECTORS - __CTORS;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "ctors", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 5: /* data */
		mrd[i].pbase = __DATA;
		mrd[i].vbase = __DATA;
		mrd[i].len   = __EDATA - __DATA;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "data", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 6: /* bss */
		mrd[i].pbase = __BSS_START;
		mrd[i].len   = __END - __BSS_START;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "bss", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 7: /* heap */
		mrd[i].pbase = _libkvmplat_cfg.heap.start;
		mrd[i].vbase = _libkvmplat_cfg.heap.start;
		mrd[i].len   = _libkvmplat_cfg.heap.len;
		mrd[i].type  = UKPLAT_MEMRT_FREE;
		mrd[i].flags = 0;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "heap", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 8: /* stack */
		mrd[i].pbase = _libkvmplat_cfg.bstack.start;
		mrd[i].vbase = _libkvmplat_cfg.bstack.start;
		mrd[i].len   = _libkvmplat_cfg.bstack.len;
		mrd[i].type  = UKPLAT_MEMRT_STACK;
		mrd[i].flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "bstack", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 9: /* initrd */
		if (_libkvmplat_cfg.initrd.len) {
			mrd[i].pbase = _libkvmplat_cfg.initrd.start;
			mrd[i].vbase = _libkvmplat_cfg.initrd.start;
			mrd[i].len   = _libkvmplat_cfg.initrd.len;
			mrd[i].type  = UKPLAT_MEMRT_INITRD;
			mrd[i].flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;
#if CONFIG_UKPLAT_MEMRNAME
			strncpy(mrd[i].name, "initrd", sizeof(mrd[i].name) - 1);
#endif
			*m = &mrd[i];
			break;
		}
		/* fall-through */
	case 10: /* heap2
		  * NOTE: heap2 could only exist if initrd was there,
		  * otherwise we fall through
		  */
		if (_libkvmplat_cfg.initrd.len && _libkvmplat_cfg.heap2.len) {
			mrd[i].pbase = _libkvmplat_cfg.heap2.start;
			mrd[i].vbase = _libkvmplat_cfg.heap2.start;
			mrd[i].len   = _libkvmplat_cfg.heap2.len;
			mrd[i].type  = UKPLAT_MEMRT_FREE;
			mrd[i].flags = 0;
#if CONFIG_UKPLAT_MEMRNAME
			strncpy(mrd[i].name, "heap", sizeof(mrd[i].name) - 1);
#endif
			*m = &mrd[i];
			break;
		}
		/* fall-through */
	default:
		return -1;
	}

	return 0;
}

int _ukplat_mem_mappings_init(void)
{
	return 0;
}
