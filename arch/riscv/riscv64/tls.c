/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Florian Schmidt <florian.schmidt@neclab.eu>
 *          Cyril Soldani <cyril.soldani@uliege.be>
 *          Simon Kuenzer <simon@unikraft.io>
 *          Robert Kuban <robert.kuban@opensynergy.com>
 *	    Eduard Vintilă <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2021, University of Liège. All rights reserved.
 * Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH All rights reserved.
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CONFIG_LIBCONTEXT_CLEAR_TBSS
#define CONFIG_LIBCONTEXT_CLEAR_TBSS 1
#endif /* CONFIG_LIBCONTEXT_CLEAR_TBSS */

#include <string.h>
#include <uk/print.h>
#include <uk/hexdump.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/arch/types.h>
#include <uk/arch/tls.h>


#ifndef __UKARCH_TLS_HAVE_TCB__
#ifndef TCB_SIZE
#define TCB_SIZE 0
#endif /* !TCB_SIZE */
#endif /* !__UKARCH_TLS_HAVE_TCB__ */

extern char _tls_start[], _etdata[], _tls_end[];

/*
 * This file implements a static exec TLS layout
 * (variant 1 with one static TLS block).
 *
 *
 * tls_area -> +-------------------------+ |  /
 *	       |    / PADDING /            |  > tls_padding_size()
 *    tcbp --> +-------------------------+ |  \
 *             |                         | |  |
 *             | Custom TCB format       | |   > Thread Control Block (TCB)
 *             | (might be used          | |  |  (length: TCB_SIZE)
 *             |  by a libC)             | |  |
 *             |                         | |  |
 *    tlsp --> +-------------------------+ |  \
 *             |                         | |  |
 *             | .tdata                  | |  |
 *             |                         | |   > Static TLS block
 *             +-------------------------+ |  |
 *             |                         | |  |
 *             | .tbss                   | |  |
 *             |                         | |  |
 *             +-------------------------+ /  /
 *
 */

/*
 * The TLS area alignment is usually read from the ELF header (p_align of the
 * TLS section).
 */
#define TLS_AREA_ALIGN 16

static __sz tls_tdata_size(void)
{
	return (__uptr) _etdata  - (__uptr) _tls_start;
}
static __sz tls_tbss_size(void)
{
	return (__uptr) _tls_end - (__uptr) _etdata;
}

static __sz tls_padding_size(void)
{
	const __sz tcb_before_tlsp = TCB_SIZE;

	return ALIGN_UP(tcb_before_tlsp, TLS_AREA_ALIGN) - tcb_before_tlsp;
}

__sz ukarch_tls_area_align(void)
{
	return TLS_AREA_ALIGN;
}

__sz ukarch_tls_area_size(void)
{
	return tls_padding_size()
		+ ukarch_tls_tcb_size()
		+ tls_tdata_size()
		+ tls_tbss_size();
}

__uptr ukarch_tls_tlsp(void *tls_area)
{
	return ((__uptr) tls_area)
		+ tls_padding_size()
		+ ukarch_tls_tcb_size();
}

void *ukarch_tls_area_get(__uptr tlsp)
{
	/* inverse of ukarch_tls_tlsp */
	return (void *) (tlsp
		- ukarch_tls_tcb_size()
		- tls_padding_size());
}

/* arch pointer to tcb pointer */
void *ukarch_tls_tcb_get(__uptr tlsp)
{
	return (void *) (tlsp
		- ukarch_tls_tcb_size());
}

__sz ukarch_tls_tcb_size(void)
{
	return TCB_SIZE;
}

void ukarch_tls_area_init(void *tls_area)
{
	const __sz padding = tls_padding_size();
	const __sz tcb_len = ukarch_tls_tcb_size();
	const __sz tdata_len = tls_tdata_size();
	const __sz tbss_len  = tls_tbss_size();
	__u8 *writepos = tls_area;

	UK_ASSERT(IS_ALIGNED((__uptr) tls_area, ukarch_tls_area_align()));
	uk_pr_debug("tls_area_init: target: %p (%"__PRIsz" bytes)\n",
		    tls_area, ukarch_tls_area_size());

	/* padding */
	uk_pr_debug("tls_area_init: pad: %"__PRIsz" bytes\n",
		    padding);
#if CONFIG_LIBCONTEXT_CLEAR_TBSS
	memset(writepos, 0x0, padding);
#endif /* CONFIG_LIBCONTEXT_CLEAR_TBSS */
	writepos += tls_padding_size();

	/* TCB */
	uk_pr_debug("tls_area_init: tcb: %"__PRIsz" bytes\n",
		    tcb_len);
	UK_ASSERT(ukarch_tls_tcb_get(ukarch_tls_tlsp(tls_area)) == writepos);
#if CONFIG_UKARCH_TLS_HAVE_TCB
	ukarch_tls_tcb_init(writepos);
#else /* !CONFIG_UKARCH_TLS_HAVE_TCB */
	memset(writepos, 0x0, tcb_len);
#endif /*!CONFIG_UKARCH_TLS_HAVE_TCB */
	writepos += tcb_len;



	/* .tdata */
	uk_pr_debug("tls_area_init: copy (.tdata): %"__PRIsz" bytes\n",
		    tdata_len);
	UK_ASSERT(IS_ALIGNED((__uptr) writepos, ukarch_tls_area_align()));
	memcpy(writepos, _tls_start, tdata_len);
	writepos += tdata_len;

	/* .tbss */
	uk_pr_debug("tls_area_init: uninitialized (.tbss): %"__PRIsz" bytes\n",
		    tbss_len);
#if CONFIG_LIBCONTEXT_CLEAR_TBSS
	memset(writepos, 0x0, tbss_len);
#endif /* CONFIG_LIBCONTEXT_CLEAR_TBSS */
	writepos += tbss_len;

	UK_ASSERT(ukarch_tls_area_size() ==
		  (__sz) (writepos - (__u8 *) tls_area));
	uk_hexdumpCd(tls_area, ukarch_tls_area_size());
}

