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

#ifndef __UKDEBUG_CRASHDUMP_H__
#define __UKDEBUG_CRASHDUMP_H__

#include <uk/arch/lcpu.h>
#include <uk/essentials.h>
#include <uk/print.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (defined UK_DEBUG) || CONFIG_LIBUKDEBUG_PRINTD
/* Please use uk_crashdumpd() instead */
void _uk_crashdumpd(__u16 libid, const char *srcname,
		    unsigned int srcline, struct __regs *regs);

/**
 * Prints a crash dump to debug output based on the registers in the register
 * snapshot.
 *
 * @param regs Register snapshot
 */
#define uk_crashdumpd(regs)                                                    \
	_uk_crashdumpd(uk_libid_self(), __STR_BASENAME__, __LINE__, (regs))

#else /* (defined UK_DEBUG) || CONFIG_LIBUKDEBUG_PRINTD */
static inline void uk_crashdumpd(struct __regs *regs __unused)
{}
#endif /* (defined UK_DEBUG) || CONFIG_LIBUKDEBUG_PRINTD */

#if CONFIG_LIBUKDEBUG_PRINTK
/* Please use uk_crashdumpk() instead */
void _uk_crashdumpk(int lvl, __u16 libid, const char *srcname,
		    unsigned int srcline, struct __regs *regs);

/**
 * Prints a crash dump to kernel output based on the registers in the register
 * snapshot.
 *
 * @param lvl Debug level
 * @param regs Register snapshot
 */
#define uk_crashdumpkr(lvl, regs)                                              \
	do {                                                                   \
		if ((lvl) <= KLVL_MAX)                                         \
			_uk_crashdumpk((lvl), uk_libid_self(),                 \
				       __STR_BASENAME__, __LINE__, (regs));    \
	} while (0)

#else /* CONFIG_LIBUKDEBUG_PRINTK */
static inline void uk_crashdumpk(int lvl __unused,
				 struct __regs *regs __unused)
{}
#endif /* CONFIG_LIBUKDEBUG_PRINTK */

#ifdef __cplusplus
}
#endif

#endif /* __UKDEBUG_CRASHDUMP_H__ */
