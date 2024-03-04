/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Author(s): Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
	unsigned long esr;
	int el;
	int reason;
	struct lcpu *lcpu; /* The LCPU that faulted */
	unsigned long far;
};

#endif /* !__ASSEMBLY__ */

/*
 * An arm64 platform library may define events for the following traps. Use
 * UK_EVENT_HANDLER(UKARCH_TRAP_*) to register a handler for a trap event.
 */
#define UKARCH_TRAP_INVALID_OP		trap_invalid_op
#define UKARCH_TRAP_DEBUG		trap_debug

#define UKARCH_TRAP_PAGE_FAULT		trap_page_fault
#define UKARCH_TRAP_BUS_ERROR		trap_bus_error

#define UKARCH_TRAP_MATH		trap_math

#define UKARCH_TRAP_SECURITY		trap_security

#define UKARCH_TRAP_SYSCALL		trap_syscall
