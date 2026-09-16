/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * uk_xc_arinc653.h : ARINC653 scheduler hypercall interface.
 * Provides xc_arinc653 hypercall interfaces.
 */

#ifndef _UK_XC_ARINC653_H_
#define _UK_XC_ARINC653_H_

#include <uk/arch/types.h>
#include <uk_xc_sysctl.h>

/**
 * uk_xc_sched_arinc653_schedule_set() - Set ARINC653 schedule for a cpupool
 * @param cpupool_id
 *	Target CPU pool ID
 * @param schedule
 *	Pointer to schedule structure to set (must be hypervisor-accessible)
 *
 * @return 0 on success, negative Xen error code on failure
 */
int uk_xc_sched_arinc653_schedule_set(
	__u32 cpupool_id,
	struct xen_sysctl_arinc653_schedule *schedule);

/**
 * uk_xc_sched_arinc653_schedule_get() - Get ARINC653 schedule for a cpupool
 * @param cpupool_id
 *	Target CPU pool ID
 * @param schedule
 *	Pointer to schedule structure to be filled by the hypervisor
 *	(must be hypervisor-accessible)
 *
 * @return 0 on success, negative Xen error code on failure
 */
int uk_xc_sched_arinc653_schedule_get(
	__u32 cpupool_id,
	struct xen_sysctl_arinc653_schedule *schedule);

#endif /* _UK_XC_ARINC653_H_ */
