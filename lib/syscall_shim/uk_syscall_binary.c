/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Binary system call handler (Linux ABI)
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
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

#include <uk/syscall.h>
#include <uk/plat/syscall.h>
#include <uk/arch/ctx.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include "arch/regmap_linuxabi.h"

void ukplat_syscall_handler(struct __regs *r)
{
	/* Place backup of extended register state on stack */
	__sz ectx_align = ukarch_ectx_align();
	__u8 ectxbuf[ukarch_ectx_size() + ectx_align];
	struct ukarch_ectx *ectx = (struct ukarch_ectx *)
					 ALIGN_UP((__uptr) ectxbuf, ectx_align);

	UK_ASSERT(r);

	/* Save extended register state */
	ukarch_ectx_store(ectx);

	uk_pr_debug("Binary system call request \"%s\" (%lu) at ip:%p (arg0=0x%lx, arg1=0x%lx, ...)\n",
		    uk_syscall_name(r->rsyscall), r->rsyscall,
		    (void *) r->rip, r->rarg0, r->rarg1);
	r->rret0 = uk_syscall6_r(r->rsyscall,
				 r->rarg0, r->rarg1, r->rarg2,
				 r->rarg3, r->rarg4, r->rarg5);

	/* Restore extended register state */
	ukarch_ectx_load(ectx);
}
