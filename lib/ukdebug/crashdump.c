/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Author(s): Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
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

#include <uk/crashdump.h>
#include "crashdump.h"
#include <uk/config.h>
#include <uk/hexdump.h>
#include "outf.h"

#include <uk/nofault.h>

#define _CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK_FMT \
	(UK_HXDF_ADDR | UK_HXDF_GRPQWORD | UK_HXDF_ASCIISEC)

void cdmp_gen_print_stack(struct out_dev *o, unsigned long sp)
{
	char buf[64];
	size_t ul_len = sizeof(unsigned long);
	int i;

	outf(o, "Stack:\n");

	/* Print one line at a time to keep the buffer small and independent
	 * of the number of words to print. In addition, we can print at least
	 * some of the memory, if the full range is not readable.
	 */
	for (i = 0; i < CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK_WORDS; i++,
		sp += ul_len) {

		if (uk_nofault_probe_r(sp, ul_len, 0) != ul_len) {
			outf(o, " %016lx  Bad stack address\n", sp);
			break;
		}

		uk_hexdumpsn(buf, sizeof(buf), (void*)sp, ul_len,
			     sp, _CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK_FMT, 1,
			     " ");

		outf(o, buf);
	}
}

#if defined(CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE) && !__OMIT_FRAMEPOINTER__
void cdmp_gen_print_call_trace_entry(struct out_dev *o, unsigned long addr)
{
#if 0
	struct uk_symbol sym;
#endif

	outf(o, " [0x%016lx]", addr);

	/* FIXME: Symbol resolution support */
#if 0
	if (uk_resolve_symbol((void *)addr, &sym)) {
		if (sym.name_len > 0) {
			outf(o, " %s+%lx", sym.name, addr - sym.addr);
		} else {
			outf(o, " %lx+%lx", sym.addr, addr - sym.addr);
		}
	}
#endif

	outf(o, "\n");
}
#endif /* defined(CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE) &&
	* !__OMIT_FRAMEPOINTER__ */

static void cdmp_crashdump(struct out_dev *o, struct __regs *regs)
{
#ifdef CONFIG_LIBUKDEBUG_CRASH_PRINT_REGISTERS
	cdmp_arch_print_registers(o, regs);
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_REGISTERS */

#ifdef CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK
	cdmp_arch_print_stack(o, regs);
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK */

#if defined(CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE) && !__OMIT_FRAMEPOINTER__
	cdmp_arch_print_call_trace(o, regs);
#endif
}

void _uk_crashdumpd(__u16 libid, const char *srcname,
		    unsigned int srcline, struct __regs *regs)
{
	struct out_dev o;

	out_dev_init_debug(&o, libid, srcname, srcline);
	return cdmp_crashdump(&o, regs);
}

#ifdef CONFIG_LIBUKDEBUG_PRINTK
void _uk_crashdumpk(int lvl, __u16 libid, const char *srcname,
		    unsigned int srcline, struct __regs *regs)
{
	struct out_dev o;

	out_dev_init_kern(&o, lvl, libid, srcname, srcline);
	return cdmp_crashdump(&o, regs);
}
#endif /* CONFIG_LIBUKDEBUG_PRINTK */
