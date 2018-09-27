/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <stdlib.h>
#include <errno.h>
#include <uk/plat/lcpu.h>
#include <_time.h>
#include <linuxu/time.h>
#include <linuxu/syscall.h>
#include <uk/print.h>

static void do_pselect(struct k_timespec *timeout)
{
	int ret;
	int nfds = 0;
	k_fd_set *readfds = NULL;
	k_fd_set *writefds = NULL;
	k_fd_set *exceptfds = NULL;

	ret = sys_pselect6(nfds, readfds, writefds, exceptfds, timeout, NULL);
	if (ret < 0 && ret != -EINTR)
		uk_pr_warn("Failed to halt LCPU: %d\n", ret);
}

void halt(void)
{
	do_pselect(NULL);
}

void time_block_until(__snsec until)
{
	struct k_timespec timeout;
	__nsec now = ukplat_monotonic_clock();

	if (until < 0 || (__nsec) until < now)
		return; /* timeout expired already */

	until -= now;
	timeout.tv_sec  = until / ukarch_time_sec_to_nsec(1);
	timeout.tv_nsec = until % ukarch_time_sec_to_nsec(1);

	do_pselect(&timeout);
}
