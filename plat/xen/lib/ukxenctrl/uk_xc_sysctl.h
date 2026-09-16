/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * uk_xc_sysctl.h : General sysctl hypercall interface.
 */

#ifndef _UK_XC_SYSCTL_H_
#define _UK_XC_SYSCTL_H_

#include <stdint.h>
/*
 * WARNING: This header defines __XEN_TOOLS__ as required by xen/sysctl.h
 * to access the Xen sysctl interface. Any file including
 * this header must be aware that it opens up the Xen sysctl tools-level
 * interface.
 */
#ifndef __XEN_TOOLS__
#define __XEN_TOOLS__
#endif
#include <xen/sysctl.h>

/*
 * Execute a Xen sysctl hypercall
 */
int uk_xc_do_sysctl(struct xen_sysctl *sysctl);

#endif /* _UK_XC_SYSCTL_H_ */
