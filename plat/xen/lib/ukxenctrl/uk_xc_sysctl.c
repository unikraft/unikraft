/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * uk_xc_sysctl.c : General sysctl hypercall interface.
 */

#include <uk/print.h>
#include <uk_xc_sysctl.h>

#if (defined __ARM_32__) || (defined __ARM_64__)
#include <xen-arm/hypercall.h>
#elif defined __X86_64__
#include <xen-x86/hypercall.h>
#else
#error "Unsupported architecture"
#endif

int uk_xc_do_sysctl(struct xen_sysctl *sysctl)
{
	int ret;

	sysctl->interface_version = XEN_SYSCTL_INTERFACE_VERSION;
	ret = HYPERVISOR_sysctl((unsigned long)sysctl);

	if (ret != 0)
		uk_pr_err("%s: cmd(%d) ret(%d)\n", __func__,
			  sysctl->cmd, ret);

	return ret;
}
