/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight I/O port definitions and helpers.
 *
 * Guest-to-host communication happens via OUT instructions to specific
 * I/O ports.  Each port triggers a VM exit that the host intercepts.
 *
 * Port numbers must match hyperlight-common outb.rs.
 */

#ifndef __HYPERLIGHT_OUTB_H__
#define __HYPERLIGHT_OUTB_H__

#include <uk/essentials.h>
#include <hyperlight/mem.h>

/*
 * OutBAction ports — handled by the sandbox-level outb dispatcher.
 */
#define HYPERLIGHT_PORT_LOG		99  /* structured log message */
#define HYPERLIGHT_PORT_CALL_FUNCTION	101 /* invoke a host function */
#define HYPERLIGHT_PORT_ABORT		102 /* abort with error code */
#define HYPERLIGHT_PORT_DEBUG_PRINT	103 /* single-byte debug output */

/*
 * VmAction ports — intercepted at the hypervisor level.
 */
#define HYPERLIGHT_PORT_PV_TIMER_CONFIG	107 /* configure PV timer period */
#define HYPERLIGHT_PORT_HALT		108 /* signal "done" to host */

/*
 * Write a 32-bit value to an I/O port, causing a VM exit.
 * On x86 this is the OUT instruction (port in DX, value in EAX).
 *
 * arm64 has no port I/O: Hyperlight leaves the guest physical page
 * behind HL_IO_PAGE_GVA unbacked, and a store to the port's slot faults
 * to the host as an MMIO write.  It has to be a single-register STR
 * without writeback, the form whose fault syndrome names the register,
 * and a 64-bit one: HVF hands the host the whole register, KVM the bytes
 * stored.  Under HVF the host may also run the guest on to a vCPU it
 * recreated from the few registers Hyperlight tracks, which leaves out
 * the thread pointers; the kernel's (per-CPU data) and the thread's TLS
 * are put back after the exit.
 */
static inline void hyperlight_out32(__u16 port, __u32 val)
{
#if defined(__x86_64__)
	__asm__ __volatile__("outl %0, %w1" : : "a"(val), "Nd"(port));
#elif defined(__aarch64__)
	__u64 tp0, tp1;

	__asm__ __volatile__("mrs %0, tpidr_el0\n\t"
			     "mrs %1, tpidr_el1\n\t"
			     "str %2, [%3]\n\t"
			     "msr tpidr_el0, %0\n\t"
			     "msr tpidr_el1, %1"
			     : "=&r"(tp0), "=&r"(tp1)
			     : "r"((__u64)val), "r"(HL_IO_PAGE_GVA + 8 * port)
			     : "memory");
#endif
}

/*
 * Send a single byte via DebugPrint (port 103).
 * One VM exit per byte — slow, but requires no shared memory setup.
 * Output appears on the host's stderr.
 */
static inline void hyperlight_debug_putc(char c)
{
	hyperlight_out32(HYPERLIGHT_PORT_DEBUG_PRINT, (__u32)c);
}

/*
 * Halt the guest.  The host reads the dispatch return address from
 * EAX to know where to re-enter on the next call.
 */
static inline void hyperlight_halt(void)
{
	hyperlight_out32(HYPERLIGHT_PORT_HALT, 0);
}

#endif /* __HYPERLIGHT_OUTB_H__ */
