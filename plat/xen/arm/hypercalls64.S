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


#include <uk/asm.h>
#include <xen/xen.h>

    .macro hypercall, name
    mov     x16, \name
    hvc     #0xEA1
    ret
    nop
    .endm

    .text
ENTRY(HYPERVISOR_memory_op)
    hypercall __HYPERVISOR_memory_op
ENDPROC(HYPERVISOR_memory_op)

ENTRY(HYPERVISOR_domctl)
    hypercall __HYPERVISOR_domctl
ENDPROC(HYPERVISOR_domctl)

ENTRY(HYPERVISOR_sched_op)
    hypercall __HYPERVISOR_sched_op
ENDPROC(HYPERVISOR_sched_op)

ENTRY(HYPERVISOR_console_io)
    hypercall __HYPERVISOR_console_io
ENDPROC(HYPERVISOR_console_io)

ENTRY(HYPERVISOR_xen_version)
    hypercall __HYPERVISOR_xen_version
ENDPROC(HYPERVISOR_xen_version)

ENTRY(HYPERVISOR_event_channel_op)
    hypercall __HYPERVISOR_event_channel_op
ENDPROC(HYPERVISOR_event_channel_op)

ENTRY(HYPERVISOR_physdev_op)
    hypercall __HYPERVISOR_physdev_op
ENDPROC(HYPERVISOR_physdev_op)

ENTRY(HYPERVISOR_sysctl)
    hypercall __HYPERVISOR_sysctl
ENDPROC(HYPERVISOR_sysctl)

ENTRY(HYPERVISOR_hvm_op)
    hypercall __HYPERVISOR_hvm_op
ENDPROC(HYPERVISOR_hvm_op)

ENTRY(HYPERVISOR_grant_table_op)
    hypercall __HYPERVISOR_grant_table_op
ENDPROC(HYPERVISOR_grant_table_op)

ENTRY(HYPERVISOR_vcpu_op)
    hypercall __HYPERVISOR_vcpu_op
ENDPROC(HYPERVISOR_vcpu_op)

ENTRY(HYPERVISOR_xsm_op)
    hypercall __HYPERVISOR_xsm_op
ENDPROC(HYPERVISOR_xsm_op)

