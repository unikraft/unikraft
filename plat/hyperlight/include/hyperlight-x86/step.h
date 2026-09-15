/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Cooperative "step" execution model for Hyperlight guests.
 *
 * Left to itself, a guest function runs to completion inside one VM
 * entry (see dispatch.c): the vCPU is busy until the call returns, and
 * an application that has nothing to do -- a server parked in accept(),
 * a thread in nanosleep() -- keeps the vCPU spinning in the idle loop.
 *
 * The step model instead hands the vCPU back to the host whenever the
 * unikernel scheduler would go idle, reporting how long until the next
 * timer fires.  The host re-enters the guest through the `step` guest
 * function to make further progress, and can snapshot the guest at any
 * of those boundaries: every runnable thread's context is already saved
 * in its TCB in guest memory, and the only state lost across the
 * HALT/re-entry (Hyperlight resets the registers on every entry) is the
 * transient dispatch stack, which holds nothing that must survive.
 *
 * Roles:
 *
 *   yield thread   A guest kernel thread, created at boot, that is the
 *                  "current" thread whenever the VM is halted.  It parks
 *                  until the scheduler first goes idle (boot complete),
 *                  halts the VM, and from then on every host->guest entry
 *                  runs on it: the dispatch handler calls
 *                  hyperlight_step_pump(), which switches into the idle
 *                  thread; when the run queue drains, the idle thread's
 *                  platform halt switches straight back and the pump
 *                  reports the deadline and returns to the halt.
 *
 *   resume         What the host calls instead of the first `step` after
 *                  restoring the guest from a snapshot: a step that first
 *                  puts the image right for its new host -- the CSPRNG is
 *                  reseeded so clones diverge, and the host sockets are
 *                  re-established (listeners bound again, connections
 *                  declared dead) since they belonged to the old host.
 *
 *   /dev/hlcall    Named guest functions (anything but `step` and `resume`)
 *                  cannot run
 *                  on the yield thread -- a call that blocks there
 *                  could never yield the vCPU.  They are queued on this
 *                  character device instead; a driver blocks in read()
 *                  on it and runs each call on its own schedulable
 *                  thread.  Re-entering read() marks the call complete.
 *
 * The guest reports to the host through named host functions, each one
 * fact with typed arguments: Yield(ns) at every boundary, DriverReady(),
 * CallStarted(), CallDone(status), CallRejected() and Exited(status)
 * (see step.c).  A halt with neither a Yield nor an Exited means the
 * process is gone with unknown status.
 */

#ifndef __HYPERLIGHT_X86_STEP_H__
#define __HYPERLIGHT_X86_STEP_H__

#include <uk/arch/types.h>
#include <uk/config.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ioctl on /dev/hlcall: store the largest call a read() can return, as a
 * __u64, at the argument.  A driver sizes its read buffer with it, so the
 * limit is set once, by the host (the PEB input stack size), and nobody
 * else hard-codes it.  The number spells 'H','L',1.
 */
#define HLCALL_IOC_MAXLEN 0x484c0001UL

#if CONFIG_HYPERLIGHT_STEP

/**
 * Run one cooperative step: dispose of the in-flight FunctionCall, drive
 * the scheduler until it would go idle, then report the next-wakeup
 * deadline to the host via the `Yield` host function and return so the
 * dispatch handler can push the void result and halt.
 *
 * @param fc      The FunctionCall FlatBuffer that entered the guest
 *                (a stable copy; the PEB input stack is reused by host
 *                calls made while the scheduler runs).
 * @param fc_len  Length of @fc.
 */
void hyperlight_step_pump(const __u8 *fc, __u64 fc_len);

/**
 * Platform halt hook.
 *
 * With a pump in flight, the idle thread reaching a halt means the run
 * queue has drained: record @wakeup_time as the deadline to report and
 * switch back to the yield thread.  Returns when the host re-enters the
 * guest and the next pump switches into the idle thread again.
 *
 * Without a pump (boot), the first idle wakes the parked yield thread so
 * it can hand the VM to the host with boot complete.
 *
 * @param wakeup_time Absolute monotonic-clock deadline, or 0 if none.
 * @return Non-zero if the halt was handled and the caller should return
 *         to its idle loop; zero if the caller must halt/spin itself.
 */
int hyperlight_step_halt(__nsec wakeup_time);

/**
 * Whether a pump is currently driving the scheduler, i.e. the VM entered
 * through the `step` model and any halt should first push a result.
 */
int hyperlight_step_active(void);

/**
 * Report the process exit status to the host (the `Exited` host
 * function).  Called by the platform shutdown path before the final
 * halt, whether or not a pump is in flight.
 */
void hyperlight_step_report_exit(void);

/**
 * True if the size-prefixed FunctionCall @fc is the pump's own `step`
 * entry point rather than a named application call.
 */
int hyperlight_step_fc_is_pump(const __u8 *fc, __u64 fc_len);

#else /* !CONFIG_HYPERLIGHT_STEP */

static inline int hyperlight_step_halt(__nsec wakeup_time __unused)
{
	return 0;
}

static inline int hyperlight_step_active(void)
{
	return 0;
}

static inline void hyperlight_step_report_exit(void)
{
}

static inline int hyperlight_step_fc_is_pump(const __u8 *fc __unused,
					     __u64 fc_len __unused)
{
	return 0;
}

#endif /* !CONFIG_HYPERLIGHT_STEP */

#ifdef __cplusplus
}
#endif

#endif /* __HYPERLIGHT_X86_STEP_H__ */
