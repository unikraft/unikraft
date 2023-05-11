/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKARCH_TRAPS_H__
#error Do not include this header directly
#endif

#ifndef __ASSEMBLY__

/**
 * This structure stores trap context information. It is supplied as data
 * for trap event handlers.
 */
struct ukarch_trap_ctx {
	struct __regs *regs;
	int trapnr;
	const char *str;
	int error_code; /* exception error code (if DECLARE_TRAP_EC()) */
	int handler_err; /* set by handler if unable to process exception */
	__vaddr_t fault_address; /* for page faults */
};

#endif /* !__ASSEMBLY__ */

/*
 * An x86 platform library may define events for the following traps. Use
 * UK_EVENT_HANDLER(UKARCH_TRAP_*) to register a handler for a trap event.
 */
#define UKARCH_TRAP_INVALID_OP		trap_invalid_op
#define UKARCH_TRAP_DEBUG		trap_debug
#define UKARCH_TRAP_NMI			trap_nmi

#define UKARCH_TRAP_PAGE_FAULT		trap_page_fault
#define UKARCH_TRAP_BUS_ERROR		trap_bus_error

#define UKARCH_TRAP_MATH		trap_math

#define UKARCH_TRAP_SECURITY		trap_security

#define UKARCH_TRAP_X86_GP		trap_gp_fault
