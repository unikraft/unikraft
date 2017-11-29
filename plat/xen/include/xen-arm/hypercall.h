/* SPDX-License-Identifier: MIT */
/******************************************************************************
 * hypercall-arm.h
 *
 * Copied from XenLinux.
 *
 * Copyright (c) 2002-2004, K A Fraser
 *
 * 64-bit updates:
 *   Benjamin Liu <benjamin.liu@intel.com>
 *   Jun Nakajima <jun.nakajima@intel.com>
 *
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
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

#ifndef __HYPERCALL_ARM_H__
#define __HYPERCALL_ARM_H__

#include <uk/arch/limits.h>
#include <xen/xen.h>
#include <xen/sched.h>
#include <xen/nmi.h>
#include <xen/xsm/flask_op.h>

int HYPERVISOR_sched_op(int cmd, void *arg);

static inline int HYPERVISOR_shutdown(unsigned int reason)
{
	struct sched_shutdown shutdown = {.reason = reason};

	return (int)HYPERVISOR_sched_op(SCHEDOP_shutdown, &shutdown);
}

int HYPERVISOR_memory_op(unsigned int cmd, void *arg);

int HYPERVISOR_event_channel_op(int cmd, void *op);

int HYPERVISOR_xen_version(int cmd, void *arg);

int HYPERVISOR_console_io(int cmd, int count, char *str);

int HYPERVISOR_physdev_op(void *physdev_op);

int HYPERVISOR_grant_table_op(unsigned int cmd, void *uop, unsigned int count);

int HYPERVISOR_vcpu_op(int cmd, int vcpuid, void *extra_args);

int HYPERVISOR_sysctl(unsigned long op);

int HYPERVISOR_domctl(unsigned long op);

int HYPERVISOR_hvm_op(unsigned long op, void *arg);

int HYPERVISOR_xsm_op(struct xen_flask_op *);

#endif /* __HYPERCALL_ARM_H__ */
