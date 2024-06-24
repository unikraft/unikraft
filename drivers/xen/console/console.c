/* SPDX-License-Identifier: BSD-3-Clause and MIT */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation.
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
#include <string.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <uk/console.h>
#include <uk/arch/limits.h>
#include <uk/assert.h>
#include <errno.h>

#include "hv_console.h"
#include "emg_console.h"

/*
 * Return the "best case" of the two return codes ret and ret2.
 */
static inline int returncode(int ret, int ret2)
{
	if (ret < 0)
		return (ret2 != -ENOTSUP) ? ret2 : ret;
	return MAX((ret), (ret2));
}

void prepare_console(void)
{
	hv_console_prepare();
}

void flush_console(void)
{
	hv_console_flush();
}

__ssz console_out(struct uk_console *dev __unused, const char *str, __sz len)
{
	int ret, ret2;

	UK_ASSERT(len <= __I_MAX);

	if (unlikely(len == 0))
		len = strnlen(str, len);

	ret  = emg_console_output(str, len);
	ret2 = hv_console_output(str, len);
	return returncode(ret, ret2);
}

__ssz console_in(struct uk_console *dev __unused, char *str __maybe_unused,
		 __sz maxlen __maybe_unused)
{
	UK_ASSERT(maxlen <= __I_MAX);
	return hv_console_input(str, maxlen);
}

static struct uk_console_ops console_ops = {
	.out = console_out,
	.in = console_in
};

static struct uk_console console_dev = UK_CONSOLE("Xen", &console_ops,
						  UK_CONSOLE_FLAG_STDOUT |
						  UK_CONSOLE_FLAG_STDIN);

void init_console(void)
{
	hv_console_init();
	uk_console_register(&console_dev);
}
