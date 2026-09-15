/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Cooperative "step" execution model for Hyperlight guests.
 * See include/hyperlight-x86/step.h for the design.
 */

#include <string.h>
#include <errno.h>
#include <uk/arch/types.h>
#include <uk/arch/time.h>
#include <uk/plat/time.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/init.h>
#include <uk/lcpu.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/sched_impl.h>
#include <uk/thread.h>
#if CONFIG_LIBDEVFS
#include <vfscore/uio.h>
#include <devfs/device.h>
#endif /* CONFIG_LIBDEVFS */

#if CONFIG_LIBUKRANDOM
#include <uk/random.h>
#endif /* CONFIG_LIBUKRANDOM */

#include <hyperlight-x86/dispatch.h>
#include <hyperlight-x86/hcall.h>
#include <hyperlight-x86/step.h>

/* Provided by shutdown.c: the raw port-108 halt with the dispatch entry
 * in RAX, without the shutdown machinery (no term functions, no result).
 */
extern void hyperlight_halt_to_host(void) __noreturn;

#ifdef CONFIG_LIBHOSTSOCK
/* Re-poll every host-proxied socket and post readiness events, waking any
 * thread parked on one (a blocking recv/accept that returned EAGAIN and
 * yielded via uk_file_poll).  Defined in lib/hostsock.
 */
extern int hostsock_rescan_events(void);
/* Re-create the guest's host sockets after a snapshot restore. */
extern void hostsock_resume(void);
#endif /* CONFIG_LIBHOSTSOCK */

/* The relative delay reported when a deadline fell due while control was
 * switching back to the yield thread.  The protocol is "0 = no timer, N =
 * re-enter within N ns", so the smallest positive delay says "already due,
 * come straight back"; the host needs no matching constant.
 */
#define HL_STEP_DUE_NS 1ULL

/* Flags reported to the host with every step (the `StepYield` host
 * function's second argument), and once at boot complete.
 */
#define HL_STEP_CALL_DONE	(1 << 0) /* a queued named call finished */
#define HL_STEP_CALL_REJECTED	(1 << 1) /* a named call had no reader */
#define HL_STEP_HAS_DRIVER	(1 << 2) /* /dev/hlcall has been opened */
#define HL_STEP_CALL_FAILED	(1 << 3) /* the driver reported the call failed */
#define HL_STEP_EXITED		(1 << 4) /* the process exited; ns = its status */

/* The guest function that drives the scheduler, and the one the host
 * uses in its place for the first entry after restoring this guest from a
 * snapshot.  Any other name is an application-level call for the
 * /dev/hlcall reader.
 */
static const char hl_step_fn_name[] = "step";
static const char hl_resume_fn_name[] = "resume";

/* ── FlatBuffer helpers (FunctionCall.function_name) ─────────────── */

static inline __u32 fb_u32(const __u8 *b, __u64 o)
{
	return b[o] | ((__u32)b[o + 1] << 8) |
	       ((__u32)b[o + 2] << 16) | ((__u32)b[o + 3] << 24);
}

static inline __u16 fb_u16(const __u8 *b, __u64 o)
{
	return b[o] | ((__u16)b[o + 1] << 8);
}

/* True if the size-prefixed FunctionCall in @b is named @name. */
static int fc_name_is(const __u8 *b, __u64 len, const char *name)
{
	__u64 root, vt, p, nlen;
	__u16 vsize, off;

	if (!b || len < 8)
		return 0;
	root = 4 + fb_u32(b, 4);
	if (root + 4 > len)
		return 0;
	/* vtable is at root - soffset; function_name is vtable slot 4. */
	vt = root - (__s32)fb_u32(b, root);
	if (vt + 6 > len)
		return 0;
	vsize = fb_u16(b, vt);
	if (vsize < 6)
		return 0;
	off = fb_u16(b, vt + 4);
	if (!off)
		return 0;
	p = root + off;
	if (p + 4 > len)
		return 0;
	p += fb_u32(b, p);
	if (p + 4 > len)
		return 0;
	nlen = fb_u32(b, p);
	if (p + 4 + nlen > len || nlen != strlen(name))
		return 0;

	return !memcmp(b + p + 4, name, nlen);
}

static void hl_step_report(__u64 ns, int flags);

/* ── Pump state ──────────────────────────────────────────────────── */

/* The yield thread: the guest thread that is "current" whenever the VM
 * is halted.  Created at boot, parked until the first idle, and from
 * then on the thread every host entry runs on (see the header).
 */
static struct uk_thread *hl_yield_thread;

/* Set once the yield thread has performed the boot-complete halt. */
static int hl_boot_halted;

/* Meaningful only while a pump is in flight. */
static int hl_pump_active;
static struct uk_thread *hl_pump_idle;
static __nsec hl_pump_wakeup;

/* ── Named-call queue (/dev/hlcall) ──────────────────────────────── */

/* Sized from the PEB input stack (hyperlight_dispatch_max_call) at init:
 * the host decides how large a call can be, the guest never guesses.
 */
static __u8 *hl_call_buf;
static __u64 hl_call_cap;
static __u64 hl_call_len;              /* 0 = nothing queued */
static struct uk_thread *hl_call_reader; /* thread blocked in read() */
static int hl_call_opened;             /* a driver has opened the device */
static int hl_call_status;             /* CALL_* bits, consumed by the pump report */

/* Queue a named FunctionCall for the device reader.
 *
 * A guest that never opens the device (a plain Linux binary run under the
 * pump) has nobody to serve the call; report it rejected at once so the
 * host fails the call instead of waiting forever for a reader.
 */
static void hl_route_call(const __u8 *fc, __u64 fc_len)
{
	if (!hl_call_opened) {
		uk_pr_warn("hyperlight: named call rejected, no /dev/hlcall reader\n");
		hl_call_status |= HL_STEP_CALL_REJECTED;
		return;
	}
	if (unlikely(!hl_call_buf || fc_len > hl_call_cap)) {
		uk_pr_err("hyperlight: cannot queue a %llu-byte named call\n",
			  (unsigned long long)fc_len);
		hl_call_status |= HL_STEP_CALL_REJECTED;
		return;
	}
	/* One call at a time: the host drives a named call to completion
	 * before issuing the next, so an unconsumed one here is a host bug.
	 */
	if (unlikely(hl_call_len)) {
		uk_pr_warn("hyperlight: named call dropped, previous one not yet consumed\n");
		return;
	}
	memcpy(hl_call_buf, fc, fc_len);
	hl_call_len = fc_len;
	if (hl_call_reader)
		uk_thread_wake(hl_call_reader);
}

#if CONFIG_LIBDEVFS
/* A call has been handed to the reader and not yet completed. */
static int hl_call_in_flight;

static int hlcall_open(struct device *dev __unused, int mode __unused)
{
	hl_call_opened = 1;
	return 0;
}

/* Block until a named call is queued, then hand its FunctionCall bytes
 * to the caller.  Coming back for the next call means the previous one
 * completed; the pump reports that on its next return.
 */
static int hlcall_read(struct device *dev __unused, struct uio *uio,
		       int flags __unused)
{
	__u8 *buf;
	__u64 cap, n;

	if (unlikely(!uio->uio_iov || uio->uio_iovcnt < 1))
		return EINVAL;
	buf = uio->uio_iov->iov_base;
	cap = uio->uio_iov->iov_len;

	if (hl_call_in_flight) {
		hl_call_in_flight = 0;
		hl_call_status |= HL_STEP_CALL_DONE;
	}

	while (!hl_call_len) {
		hl_call_reader = uk_thread_current();
		uk_thread_block(hl_call_reader);
		uk_sched_yield();
	}
	hl_call_reader = NULL;

	n = MIN(hl_call_len, cap);
	memcpy(buf, hl_call_buf, n);
	hl_call_len = 0;
	hl_call_in_flight = 1;
	uio->uio_resid -= (ssize_t)n;

	return 0;
}

/* The driver reports how the call it is serving went: a 32-bit status,
 * 0 for success.  Written before the next read(), which marks the call
 * done; a non-zero status flags it failed in that same report.  The
 * kernel does not interpret the value beyond zero / non-zero.
 */
static int hlcall_write(struct device *dev __unused, struct uio *uio,
			int flags __unused)
{
	__s32 status;

	if (unlikely(!uio->uio_iov || uio->uio_iovcnt < 1 ||
		     uio->uio_iov->iov_len < sizeof(status)))
		return EINVAL;
	if (!hl_call_in_flight)
		return EPERM;
	memcpy(&status, uio->uio_iov->iov_base, sizeof(status));
	if (status)
		hl_call_status |= HL_STEP_CALL_FAILED;
	uio->uio_resid -= (ssize_t)sizeof(status);
	return 0;
}

/* HLCALL_IOC_MAXLEN: tell the driver how big a call can get, so it sizes
 * its read buffer from the host's number rather than a guess of its own.
 */
static int hlcall_ioctl(struct device *dev __unused, unsigned long cmd,
			void *arg)
{
	if (cmd != HLCALL_IOC_MAXLEN)
		return ENOTTY;
	if (unlikely(!arg))
		return EINVAL;
	*(__u64 *)arg = hl_call_cap;
	return 0;
}

static struct devops hlcall_devops = {
	.open  = hlcall_open,
	.close = dev_noop_close,
	.read  = hlcall_read,
	.write = hlcall_write,
	.ioctl = hlcall_ioctl,
};

static struct driver hlcall_driver = {
	.name   = "hlcall",
	.devops = &hlcall_devops,
	.devsz  = 0,
};

static int hlcall_register(struct uk_init_ctx *ictx __unused)
{
	int rc;

	rc = device_create(&hlcall_driver, "hlcall", D_CHR, NULL);
	if (unlikely(rc)) {
		uk_pr_err("hyperlight: failed to register /dev/hlcall: %d\n",
			  rc);
		return -rc;
	}
	return 0;
}

devfs_initcall(hlcall_register);
#endif /* CONFIG_LIBDEVFS */

/* ── Exit status ─────────────────────────────────────────────────── */

/* The exit status ukboot is shutting down with, captured from the term
 * context on the way down.  Term handlers run in reverse init order, so
 * this one is registered at the platform class to run after the library
 * handlers: posix-process's (a late initcall) has by then replaced
 * main()'s return value with the init process's exit status, which is
 * the one that matters under the elfloader, where main() never returns.
 */
static int hl_exit_code;

static int hl_exit_init(struct uk_init_ctx *ictx __unused)
{
	return 0;
}

static void hl_record_exit(struct uk_term_ctx *tctx)
{
	hl_exit_code = tctx->exit_code;
}

uk_plat_initcall(hl_exit_init, hl_record_exit);

void hyperlight_step_report_exit(void)
{
	hl_step_report((__u64)(__u32)hl_exit_code, HL_STEP_EXITED);
}

/* ── Yield thread ────────────────────────────────────────────────── */

static __noreturn void hl_yield_thread_fn(void)
{
	/* Park until the scheduler first goes idle (boot complete). */
	uk_thread_block(uk_thread_current());
	uk_sched_yield();

	/* Tell the host this guest runs the step model and whether a driver
	 * is listening for named calls, then hand the VM over.  Every later
	 * entry comes back through hyperlight_dispatch_function on this
	 * thread and reaches the pump.
	 */
	hl_boot_halted = 1;
	hl_step_report(0, 0);
	hyperlight_halt_to_host();
}

static int hl_yield_thread_create(struct uk_init_ctx *ictx __unused)
{
	struct uk_sched *s = uk_sched_current();

	if (!s) {
		uk_pr_debug("hyperlight: no scheduler, step model disabled\n");
		return 0;
	}

	/* The named-call queue holds one call of the largest size the host
	 * can send.  Without it every named call is rejected; the step
	 * model itself still works, so this is not fatal.
	 */
	hl_call_cap = hyperlight_dispatch_max_call();
	hl_call_buf = hl_call_cap ?
		uk_malloc(uk_alloc_get_default(), hl_call_cap) : NULL;
	if (unlikely(!hl_call_buf)) {
		uk_pr_err("hyperlight: no memory for the %llu-byte call queue\n",
			  (unsigned long long)hl_call_cap);
		hl_call_cap = 0;
	}

	/* Default stack: it only ever runs the park above; once the VM has
	 * halted on it, every entry runs on the dispatch stack instead.
	 */
	hl_yield_thread = uk_sched_thread_create_fn0(s, hl_yield_thread_fn,
						    0, 0, false, false,
						    "hl-yield", NULL, NULL);
	if (unlikely(!hl_yield_thread)) {
		uk_pr_err("hyperlight: failed to create the yield thread; "
			  "the step model is unavailable\n");
		return 0;
	}
	return 0;
}

uk_late_initcall(hl_yield_thread_create, 0x0);

/* ── Pump ────────────────────────────────────────────────────────── */

/* The host has just restored this guest from a snapshot.  Two things are
 * put right here, before anything else runs:
 *
 *   the CSPRNG is reseeded: its state lives in guest memory, so without
 *   this every clone of one snapshot would draw the same "random" bytes
 *   (the same UUIDs, tokens and TLS nonces) until the periodic reseed
 *   happened to fire;
 *
 *   the host sockets are re-established: they belonged to the process
 *   that took the snapshot, so listeners are opened and bound again and
 *   connections are declared dead (see hostsock_resume()).
 */
static void hl_resume(void)
{
#if CONFIG_LIBUKRANDOM
	int rc = uk_random_reseed();

	if (unlikely(rc))
		uk_pr_err("hyperlight: CSPRNG reseed after restore failed: %d\n",
			  rc);
#endif /* CONFIG_LIBUKRANDOM */
#ifdef CONFIG_LIBHOSTSOCK
	hostsock_resume();
#endif /* CONFIG_LIBHOSTSOCK */
}

int hyperlight_step_active(void)
{
	return hl_pump_active;
}

int hyperlight_step_fc_is_pump(const __u8 *fc, __u64 fc_len)
{
	return fc_name_is(fc, fc_len, hl_step_fn_name);
}

int hyperlight_step_halt(__nsec wakeup_time)
{
	struct uk_thread *cur = uk_thread_current();

	/* A pump is driving the scheduler and its idle thread reached a
	 * halt: the run queue has drained.  Record the deadline and hand
	 * the vCPU back; the next pump switches into us again right here.
	 */
	if (hl_pump_active && cur == hl_pump_idle) {
		hl_pump_wakeup = wakeup_time;
		uk_sched_thread_switch(hl_yield_thread);
		return 1;
	}

	/* Boot: the first idle means the workload has started and has
	 * nothing more to do until the host drives it.  Wake the parked
	 * yield thread; the idle loop yields to it and it halts the VM.
	 * (uk_thread_is_blocked() is not used: upstream's macro is
	 * mis-parenthesised and always false.)
	 */
	if (!hl_boot_halted && hl_yield_thread &&
	    !uk_thread_is_runnable(hl_yield_thread)) {
		uk_thread_wake(hl_yield_thread);
		return 1;
	}

	return 0;
}

/* Report the outcome of a step to the host.
 *
 *   ns: nanoseconds until the next guest timer fires (0 = none pending;
 *       HL_STEP_DUE_NS = a timer is already due, re-poll at once).
 *       With HL_STEP_EXITED set it carries the process exit status.
 *   flags: HL_STEP_* bits -- what became of a queued named call, whether
 *       a driver is listening on /dev/hlcall at all, and whether the
 *       process has exited.
 *
 * Failure is non-fatal: without a report the host sees the step as an
 * exit, which is the right reading of a host without `StepYield`.
 */
static void hl_step_report(__u64 ns, int flags)
{
	struct hl_param p[2];
	__s32 out;

	if (hl_call_opened)
		flags |= HL_STEP_HAS_DRIVER;

	p[0].type = HL_PV_HLULONG;
	p[0].u64_val = ns;
	p[1].type = HL_PV_HLINT;
	p[1].i32_val = flags;

	(void)hl_hcall_int("StepYield", p, 2, &out);
}

void hyperlight_step_pump(const __u8 *fc, __u64 fc_len)
{
	struct uk_sched *s = uk_sched_current();
	struct uk_thread *idle;
	unsigned long flags;
	__nsec wakeup, now;
	__u64 ns;
	int status;

	/* The const is dropped because uk_sched_thread_switch() needs a
	 * mutable handle; the idle thread object is legitimately mutable.
	 */
	idle = s ? (struct uk_thread *)uk_sched_idle_thread(s, 0) : NULL;
	if (unlikely(!idle || !hl_yield_thread ||
		     uk_thread_current() != hl_yield_thread)) {
		/* Nothing left to drive: no scheduler, or the VM last halted
		 * on some other thread -- the shutdown path after the workload
		 * exited during boot, before the yield thread ever ran.  Say
		 * nothing: a step that ends without a report is how the host
		 * learns the guest is gone, and reporting "idle" here would
		 * have it wait on a dead guest forever.
		 */
		uk_pr_warn("hyperlight: step pump entered without a "
			   "schedulable guest\n");
		return;
	}

	if (fc_name_is(fc, fc_len, hl_resume_fn_name))
		hl_resume();
	else if (!hyperlight_step_fc_is_pump(fc, fc_len))
		hl_route_call(fc, fc_len);

	hl_pump_active = 1;
	hl_pump_idle = idle;
	hl_pump_wakeup = 0;

	/* The cooperative scheduler requires IRQs enabled (schedcoop asserts
	 * it), and the dispatch entry runs with them disabled: enable them
	 * for the scheduler run and put the caller's state back after idle
	 * hands control back.
	 */
	flags = uk_lcpu_save_irqf();
	uk_lcpu_enable_irq();

#ifdef CONFIG_LIBHOSTSOCK
	/* A re-entry is the chance to observe socket I/O that arrived while
	 * the vCPU was yielded: refresh readiness on every tracked socket so
	 * a thread parked on one is woken and run below.
	 */
	hostsock_rescan_events();
#endif /* CONFIG_LIBHOSTSOCK */

	/* Switch straight into the idle thread.  It runs every runnable
	 * thread and, once the run queue drains, reaches its platform halt,
	 * which switches control back here.  Entering via idle rather than
	 * yielding guarantees the scheduler reaches idle even though this
	 * thread stays "current": it is never offered to the run queue.
	 */
	uk_sched_thread_switch(idle);

	uk_lcpu_restore_irqf(flags);

	wakeup = hl_pump_wakeup;
	status = hl_call_status;
	hl_pump_active = 0;
	hl_pump_idle = NULL;
	hl_pump_wakeup = 0;
	hl_call_status = 0;

	if (wakeup) {
		now = ukplat_monotonic_clock();
		ns = (wakeup > now) ? (__u64)(wakeup - now) : HL_STEP_DUE_NS;
	} else {
		ns = 0;
	}

	hl_step_report(ns, status);
}
