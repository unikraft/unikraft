/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Dump disassembler output to kern/debug console
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <uk/asmdump.h>
#include <uk/assert.h>
#include <errno.h>
#include "outf.h"

#if CONFIG_LIBZYDIS
#include <Zydis/Zydis.h>

/**
 * Disassemble <num_ins> instructions with zydis starting
 * with instruction at <addr>
 */
static int _asmdump(struct out_dev *o,
		   const void *instr, unsigned int count)
{
	ZydisDecoder dcr;
	ZydisFormatter fmt;
	ZydisDecodedInstruction ins;
	char buf[128];
	int offset = 0;
	int ret, total = 0;
	__uptr addr = (__uptr) instr;

#if __X86_32__
	if (!ZYAN_SUCCESS(ZydisDecoderInit(&dcr,
					   ZYDIS_MACHINE_MODE_LONG_COMPAT_32,
					   ZYDIS_ADDRESS_WIDTH_32)))
		return -1;
#elif __X86_64__
	if (!ZYAN_SUCCESS(ZydisDecoderInit(&dcr,
					   ZYDIS_MACHINE_MODE_LONG_64,
					   ZYDIS_ADDRESS_WIDTH_64)))
		return -1;
#else
#error libzydis: Unsupported architecture
#endif

	if (!ZYAN_SUCCESS(ZydisFormatterInit(&fmt,
					     ZYDIS_FORMATTER_STYLE_ATT)))
		return -1;

	while (count) {
		addr = ((__uptr) instr) + offset;
		ZydisDecoderDecodeBuffer(&dcr, (const void *) addr, 16, &ins);
		ZydisFormatterFormatInstruction(&fmt, &ins, buf, sizeof(buf),
						addr);
		ret = outf(o, "%08"__PRIuptr" <+%d>: %hs\n", addr, offset, buf);
		if (ret < 0)
			return ret;

		total += ret;
		offset += ins.length;
		count--;
	}

	return total;
}
#else /* CONFIG_LIBZYDIS */
#error No supported disassembler backend available.
#endif /* CONFIG_LIBZYDIS */

void _uk_asmdumpd(const char *libname, const char *srcname,
		  unsigned int srcline, const void *instr,
		  unsigned int instr_count)
{
	struct out_dev o;

	out_dev_init_debug(&o, libname, srcname, srcline);
	_asmdump(&o, instr, instr_count);
}

#if CONFIG_LIBUKDEBUG_PRINTK
void _uk_asmdumpk(int lvl, const char *libname,
		  const char *srcname, unsigned int srcline,
		  const void *instr, unsigned int instr_count)
{
	struct out_dev o;

	out_dev_init_kern(&o, lvl, libname, srcname, srcline);
	_asmdump(&o, instr, instr_count);
}
#endif /* CONFIG_LIBUKDEBUG_PRINTK */
