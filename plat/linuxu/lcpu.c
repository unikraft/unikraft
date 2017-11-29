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
#include <linuxu/syscall.h>
#include <uk/print.h>
#include <uk/plat/lcpu.h>

void ukplat_lcpu_halt(void)
{
	int ret;
	int nfds = 0;
	fd_set *readfds = NULL;
	fd_set *writefds = NULL;
	fd_set *exceptfds = NULL;

	ret = sys_pselect6(nfds, readfds, writefds, exceptfds, NULL, NULL);
	if (ret < 0)
		uk_printd(DLVL_WARN, "Failed to halt LCPU: %d\n", ret);
}

void ukplat_lcpu_halt_to(unsigned long millis)
{
	int ret;
	int nfds = 0;
	fd_set *readfds = NULL;
	fd_set *writefds = NULL;
	fd_set *exceptfds = NULL;
	struct timespec timeout;

	timeout.tv_sec  = millis / 1000;
	timeout.tv_nsec = millis % 1000 * 1000000;

	ret = sys_pselect6(nfds, readfds, writefds, exceptfds, &timeout, NULL);
	if (ret < 0)
		uk_printd(DLVL_WARN, "Failed to halt LCPU: %d\n", ret);
}
