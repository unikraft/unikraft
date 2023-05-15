/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>
#include <uk/print/hexdump.h>

#if CONFIG_LIBUKNOFAULT
#include <uk/nofault.h>
#endif /* CONFIG_LIBUKNOFAULT */

#include "crashdump.h"

#define STACK_WORDS CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK_WORDS
#define CRASH_PRINT_STACK_FMT				    \
	(UK_HXDF_ADDR | UK_HXDF_GRPQWORD | UK_HXDF_ASCIISEC)

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK
void cdmp_gen_print_stack(unsigned long sp)
{
	char buf[64];
	__sz ul_len = sizeof(unsigned long);
	int i;

	crash_printk("Stack:\n");

	/* Print one line at a time to keep the buffer small and independent
	 * of the number of words to print. In addition, we can print at least
	 * some of the memory, if the full range is not readable.
	 */
	for (i = 0; i < STACK_WORDS; i++, sp += ul_len) {
#if CONFIG_LIBUKNOFAULT
		if (uk_nofault_probe_r(sp, ul_len, 0) != ul_len) {
			crash_printk(" %016lx  Bad stack address\n", sp);
			break;
		}
#endif /* CONFIG_LIBUKNOFAULT */
		uk_hexdumpsn(buf, sizeof(buf), (void *)sp, ul_len,
			     sp, CRASH_PRINT_STACK_FMT, 1, " ");
		crash_printk("%s", buf);
	}
}
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK */

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE && !__OMIT_FRAMEPOINTER__
void cdmp_gen_print_call_trace_entry(unsigned long addr)
{
	crash_printk(" [0x%016lx]", addr);

	/* TODO: Symbol resolution support */

	crash_printk("\n");
}
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE && !__OMIT_FRAMEPOINTER__ */

void crash_print_crashdump(struct __regs *regs)
{
	cdmp_arch_print_registers(regs);

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK
	cdmp_arch_print_stack(regs);
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK */

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE && !__OMIT_FRAMEPOINTER__
	cdmp_arch_print_call_trace(regs);
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE && !__OMIT_FRAMEPOINTER__ */
}
