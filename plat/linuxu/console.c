/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
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

#include <inttypes.h>
#include <string.h>
#include <linuxu/syscall.h>
#include <uk/plat/console.h>
#include <uk/arch/lcpu.h>
#include <uk/assert.h>

#include <linuxu/termios.h>

#define STDIN    0
#define STDOUT   1
#define STDDEBUG 2

static struct termios t_orig;
static int cons_initialized;

void _liblinuxuplat_init_console(void)
{
	int ret;
	struct termios t;

	/* TODO: Detect that STDIN/STDOUT is a tty */

	/*
	 * Save current TTY settings
	 */
	ret = tcgetattr(STDIN, &t_orig);
	if (ret < 0)
		goto err;

	/*
	 * New terminal setting with RAW mode
	 * but keep signals and newline translations
	 */
	t = t_orig;
	termiossetraw(&t);

	/* newline translation on input */
	t.c_iflag |= ICRNL;

	/* newline translation on output (why TABDLY?) */
	t.c_oflag |= OPOST | TABDLY;

	/* enable signals from terminal (why ECHOKE?) */
	t.c_lflag |= ISIG | ECHOKE;

	/* control chars: min 1 time 0 */
	t.c_cc[VMIN]  = 1;
	t.c_cc[VTIME] = 0;

	ret = tcsetattr(STDIN, TCSAFLUSH, &t);
	if (ret < 0)
		goto err;

	cons_initialized = 1;
	return;

err:
	UK_CRASH("Failed to initialize terminal console\n");
}

void _liblinuxuplat_fini_console(void)
{
	/* restore console */
	if (cons_initialized)
		tcsetattr(STDIN, TCSAFLUSH, &t_orig);
}

int ukplat_coutd(const char *str, unsigned int len)
{
	ssize_t ret = 0;

	ret = sys_write(STDDEBUG, str, len);
	return (int) ret;
}

int ukplat_coutk(const char *str, unsigned int len)
{
	ssize_t ret = 0;

	ret = sys_write(STDOUT, str, len);
	return (int) ret;
}

int ukplat_cink(char *str, unsigned int maxlen)
{
	ssize_t ret = 0;

	ret = sys_read(STDIN, str, maxlen);
	return (int) ret;
}
