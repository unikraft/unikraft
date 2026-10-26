/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * uk_xc_arinc653.c
 * XC interface to the ARINC653 scheduler
 */

#include <uk/arch/types.h>
#include <uk/assert.h>
#include <uk_xc_sysctl.h>
#include <uk_xc_arinc653.h>

int uk_xc_sched_arinc653_schedule_set(
	__u32 cpupool_id,
	struct xen_sysctl_arinc653_schedule *schedule)
{
	struct xen_sysctl sysctl = {};

	UK_ASSERT(schedule);

	sysctl.cmd = XEN_SYSCTL_scheduler_op;
	sysctl.u.scheduler_op.cpupool_id = cpupool_id;
	sysctl.u.scheduler_op.sched_id = XEN_SCHEDULER_ARINC653;
	sysctl.u.scheduler_op.cmd = XEN_SYSCTL_SCHEDOP_putinfo;
	set_xen_guest_handle(sysctl.u.scheduler_op.u.sched_arinc653.schedule,
			     schedule);

	return uk_xc_do_sysctl(&sysctl);
}

int uk_xc_sched_arinc653_schedule_get(
	__u32 cpupool_id,
	struct xen_sysctl_arinc653_schedule *schedule)
{
	struct xen_sysctl sysctl = {};

	UK_ASSERT(schedule);

	sysctl.cmd = XEN_SYSCTL_scheduler_op;
	sysctl.u.scheduler_op.cpupool_id = cpupool_id;
	sysctl.u.scheduler_op.sched_id = XEN_SCHEDULER_ARINC653;
	sysctl.u.scheduler_op.cmd = XEN_SYSCTL_SCHEDOP_getinfo;
	set_xen_guest_handle(sysctl.u.scheduler_op.u.sched_arinc653.schedule,
			     schedule);

	return uk_xc_do_sysctl(&sysctl);
}
