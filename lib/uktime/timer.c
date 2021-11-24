/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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

#include <errno.h>
#include <time.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/syscall.h>


UK_SYSCALL_R_DEFINE(int, timer_create, clockid_t, clockid,
		    struct sigevent *__restrict, sevp,
		    timer_t *__restrict, timerid)
{
	UK_WARN_STUBBED();
	return -ENOTSUP;
}

UK_SYSCALL_R_DEFINE(int, timer_delete,
		    timer_t, timerid)
{
	UK_WARN_STUBBED();
	return -ENOTSUP;
}

UK_SYSCALL_R_DEFINE(int, timer_settime,
		    timer_t, timerid,
		    int, flags,
		    const struct itimerspec *__restrict, new_value,
		    struct itimerspec *__restrict, old_value)
{
	UK_WARN_STUBBED();
	return -ENOTSUP;
}

UK_SYSCALL_R_DEFINE(int, timer_gettime,
		    timer_t, timerid,
		    struct itimerspec *, curr_value)
{
	UK_WARN_STUBBED();
	return -ENOTSUP;
}

UK_SYSCALL_R_DEFINE(int, timer_getoverrun,
		    timer_t, timerid)
{
	UK_WARN_STUBBED();
	return -ENOTSUP;
}
