/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __HYPERLIGHT_X86_OUTB_H__
#define __HYPERLIGHT_X86_OUTB_H__

/**
 * Supported actions when issuing OUTB actions to Hyperlight host.
 * Maps to hyperlight_common::outb::OutBAction
 */
enum hyperlight_outb_action {
	HYPERLIGHT_OUTB_LOG           = 99,
	HYPERLIGHT_OUTB_CALL_FUNCTION = 101,
	HYPERLIGHT_OUTB_ABORT         = 102,
	HYPERLIGHT_OUTB_DEBUG_PRINT   = 103,
	HYPERLIGHT_OUTB_TRACE_BATCH   = 104,
	HYPERLIGHT_OUTB_TRACE_MEM_ALLOC = 105,
	HYPERLIGHT_OUTB_TRACE_MEM_FREE  = 106,
	/* VM-level actions (hypervisor intercepts, not guest OutBAction) */
	HYPERLIGHT_VM_HALT            = 108,
};

/**
 * Write a byte to an I/O port.
 * Used for communication with the Hyperlight host.
 */
static inline void hyperlight_outb(__u16 port, __u8 value)
{
	__asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Write a 32-bit value to an I/O port using OUT instruction.
 * This is used for structured communication with the host.
 */
static inline void hyperlight_out32(__u16 port, __u32 value)
{
	__asm__ __volatile__("out %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Send a debug print message to the host.
 * Each byte is sent individually via the DebugPrint port.
 */
static inline void hyperlight_debug_print(const char *msg, __sz len)
{
	for (__sz i = 0; i < len; i++) {
		hyperlight_outb(HYPERLIGHT_OUTB_DEBUG_PRINT, msg[i]);
	}
}

/**
 * Send an abort signal to the Hyperlight host.
 * This follows the Hyperlight abort protocol:
 * - Data is sent in chunks with a length prefix
 * - Each chunk: [length, byte1, byte2, byte3] as 32-bit little-endian
 * - Abort sequence: send code bytes, then 0xFF terminator
 *
 * @param code The abort code (0 for success, non-zero for error)
 */
static inline void hyperlight_abort(__u8 code)
{
	__u32 chunk;

	/* Send [code, 0xFF] - length 2, then code and terminator */
	chunk = 2 | ((__u32)code << 8) | (0xFF << 16);
	hyperlight_out32(HYPERLIGHT_OUTB_ABORT, chunk);

	/* Send terminator [0xFF] - length 1 */
	chunk = 1 | (0xFF << 8);
	hyperlight_out32(HYPERLIGHT_OUTB_ABORT, chunk);
}

#endif /* __HYPERLIGHT_X86_OUTB_H__ */
