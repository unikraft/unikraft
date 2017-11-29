/* SPDX-License-Identifier: GPL-2.0 or MIT */
/******************************************************************************
 * hypercall.S
 *
 * Xen hypercall wrappers
 *
 * Stefano Stabellini <stefano.stabellini@eu.citrix.com>, Citrix, 2012
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation; or, when distributed
 * separately from the Linux kernel or incorporated into other
 * software packages, subject to the following license:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <xen/xen.h>

#define __HVC(imm16) .long ((0xE1400070 | (((imm16) & 0xFFF0) << 4) | ((imm16) & 0x000F)) & 0xFFFFFFFF)

#define XEN_IMM 0xEA1

#define HYPERCALL_SIMPLE(hypercall)		\
.globl HYPERVISOR_##hypercall;			\
.align 4,0x90;					\
HYPERVISOR_##hypercall:				\
        mov r12, #__HYPERVISOR_##hypercall;	\
        __HVC(XEN_IMM);				\
        mov pc, lr;

#define _hypercall0 HYPERCALL_SIMPLE
#define _hypercall1 HYPERCALL_SIMPLE
#define _hypercall2 HYPERCALL_SIMPLE
#define _hypercall3 HYPERCALL_SIMPLE
#define _hypercall4 HYPERCALL_SIMPLE

_hypercall2(sched_op);
_hypercall2(memory_op);
_hypercall2(event_channel_op);
_hypercall2(xen_version);
_hypercall3(console_io);
_hypercall1(physdev_op);
_hypercall3(grant_table_op);
_hypercall3(vcpu_op);
_hypercall1(sysctl);
_hypercall1(domctl);
_hypercall2(hvm_op);
_hypercall1(xsm_op);
