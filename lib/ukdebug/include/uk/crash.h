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

#ifndef __UKDEBUG_CRASH_H__
#define __UKDEBUG_CRASH_H__

#include <uk/plat/bootstrap.h>
#include <uk/arch/lcpu.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_event_crash_parameter {
	/** Optional register state at the crash location */
	struct __regs *regs;
	/** Optional explicit crash description */
	struct uk_crash_description *descr;
};

/**
 * Event for crashes. Will receive a uk_event_crash_parameter as data argument.
 */
#define UK_EVENT_CRASH uk_event_crash

/* Use the UK_CRASH*() and UK_ASSERT() macros instead */
void
_uk_crash(struct __regs *regs, struct uk_crash_description *descr) __noreturn;

#ifdef __cplusplus
}
#endif

#endif /* __UKDEBUG_CRASH_H__ */
