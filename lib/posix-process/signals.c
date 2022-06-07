/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
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

#include <uk/config.h>
#include <uk/arch/lcpu.h>
#include <uk/process.h>
#include <uk/print.h>

#if CONFIG_LIBPOSIX_PROCESS_CLONE
static int uk_posix_clone_sighand(const struct clone_args *cl_args,
				  size_t cl_args_len __unused,
				  struct uk_thread *child __unused,
				  struct uk_thread *parent __unused)
{
	/* CLONE_SIGHAND and CLONE_CLEAR_SIGHAND should not be together */
	if (unlikely((cl_args->flags & (CLONE_SIGHAND | CLONE_CLEAR_SIGHAND))
		     == (CLONE_SIGHAND | CLONE_CLEAR_SIGHAND)))
		return -EINVAL;
	/* CLONE_SIGHAND requires CLONE_VM */
	if (unlikely((cl_args->flags & CLONE_SIGHAND)
		     && !(cl_args->flags & CLONE_VM)))
		return -EINVAL;
	/* CLONE_THREAD requires CLONE_SIGHAND */
	if (unlikely((cl_args->flags & CLONE_THREAD)
		     && !(cl_args->flags & CLONE_SIGHAND)))
		return -EINVAL;

	UK_WARN_STUBBED();
	return 0;
}

UK_POSIX_CLONE_HANDLER(CLONE_SIGHAND | CLONE_CLEAR_SIGHAND, false,
		       uk_posix_clone_sighand, 0x0);
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */
