/* SPDX-License-Identifier: MIT */
/******************************************************************************
 * panic.c
 *
 * Displays a register dump and stack trace for debugging.
 *
 * Copyright (c) 2014, Thomas Leonard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <xen-arm/os.h>
#include <uk/print.h>
#include <uk/essentials.h>

extern int irqstack[];
extern int irqstack_end[];

typedef void handler(void);

extern handler fault_reset;
extern handler fault_undefined_instruction;
extern handler fault_svc;
extern handler fault_prefetch_call;
extern handler fault_prefetch_abort;
extern handler fault_data_abort;

void dump_registers(int *saved_registers __unused)
{
	/* TODO: Point to ukarch_dump_registers */
}
