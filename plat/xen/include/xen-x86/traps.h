/* SPDX-License-Identifier: MIT */
/*
 ****************************************************************************
 * (C) 2005 - Grzegorz Milos - Intel Reseach Cambridge
 ****************************************************************************
 *
 *        File: traps.h
 *      Author: Grzegorz Milos (gm281@cam.ac.uk)
 *
 *        Date: Jun 2005
 *
 * Environment: Xen Minimal OS
 * Description: Deals with traps
 *
 ****************************************************************************
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

#ifndef _TRAPS_H_
#define _TRAPS_H_

#include <stdint.h>
#include <x86/traps.h>

#include <xen/xen.h>

#define TRAP_coproc_seg_overrun  9
#define TRAP_spurious_int        15
#define TRAP_xen_callback        32

/* Assembler stubs */
DECLARE_ASM_TRAP(coproc_seg_overrun);
DECLARE_ASM_TRAP(spurious_int);
DECLARE_ASM_TRAP(hypervisor_callback);
void asm_failsafe_callback(void);

#define __KERNEL_CS     FLAT_KERNEL_CS
#define __KERNEL_DS     FLAT_KERNEL_DS
#define __KERNEL_SS     FLAT_KERNEL_SS

#endif /* _TRAPS_H_ */
