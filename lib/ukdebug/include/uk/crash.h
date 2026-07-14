/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_CRASH_H__
#define __UK_CRASH_H__

#include <uk/lcpu.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_CRASH(fmt, ...)						\
	do {								\
		uk_pr_crit((fmt), ##__VA_ARGS__);			\
		uk_crash_trigger();					\
	} while (0)

/* Stops execution when reaching an impossible path, likely sign of a bug */
#define UK_BUG() UK_CRASH("Impossible execution path\n")

/* Conditional version of UK_BUG() */
#define UK_BUGON(x)							\
	do {								\
		if (unlikely(x))					\
			UK_BUG();					\
	} while (0)

/**
 * Triggers a system crash with the given register context and description.
 *
 * Use this macro when you need to report a crash from a location different
 * from where the error originated, preserving the original execution context.
 *
 * @param regs Register state/context at the point of failure
 * @param descr Description structure providing the reason for the crash
 */
#define UK_CRASH_EX(regs, descr)	_uk_crash(regs, descr)

enum uk_crash_reason {
	UK_CRASH_REASON_INVALID,
	/**
	 * The crash was caused by an explicit UK_CRASH
	 */
	UK_CRASH_REASON_EXPLICIT,
	/**
	 * The crash was caused by an unhandled trap.
	 */
	UK_CRASH_REASON_UNHANDLED_TRAP,
	/**
	 * The crash was caused by an unhandled page fault.
	 */
	UK_CRASH_REASON_PAGE_FAULT,
	/**
	 * The crash was caused by something else.
	 */
	UK_CRASH_REASON_OTHER,
};

struct uk_crash_descr {
	/**
	 * Contains the crash reason.
	 * The meaning of the following fields change depending on the reason.
	 */
	enum uk_crash_reason reason;

	/**
	 * Contains the error code the systems exits with.
	 */
	int uk_err;

	/**
	 * For UK_CRASH_REASON_UNHANDLED_TRAP:
	 *   the architecture specific trap number
	 * For UK_CRASH_REASON_PAGE_FAULT:
	 *   the virtual address which was attempted to access
	 */
	__u64 arg1;

	/**
	 * For UK_CRASH_REASON_UNHANDLED_TRAP:
	 *   pointer to human-readable string of the trap number
	 * For UK_CRASH_REASON_PAGE_FAULT:
	 *   an architecture specific error code
	 */
	__u64 arg2;

	/**
	 * For UK_CRASH_REASON_UNHANDLED_TRAP:
	 *   error code of the trap
	 */
	__u64 arg3;
};

/**
 * Event raised after a crashdump to give libraries a chance
 * to act after a crash. Will receive a uk_crash_event_param
 * as data argument. This separate event is required due to
 * UK_CRASH_EX() that bypasses the normal exception route.
 */
#define UK_CRASH_EVENT uk_crash_event

struct uk_crash_event_param {
	/** Optional register state at the crash location */
	struct uk_lcpu_regs *regs;
	/** Optional explicit crash description */
	struct uk_crash_descr *descr;
};

/**
 * Internal, use UK_CRASH_EX() instead
 */
void _uk_crash(struct uk_lcpu_regs *regs, struct uk_crash_descr *descr);

#ifdef __cplusplus
}
#endif

#endif /* __UK_CRASH_H__ */
