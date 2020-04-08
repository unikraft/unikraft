/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Dump disassembler output to kern/debug console
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __UKDEBUG_ASMDUMP__
#define __UKDEBUG_ASMDUMP__

/**
 * NOTE: Please note, this file defines only variants that print disassembler
 *       output to the KERN and DEBUG console: uk_asmdumpd(), ukasmdumpk().
 *       They are intended for debugging purpose only because the calls get
 *       removed if there is no supported disassembler backend available
 *       (e.g., libzydis).
 */

#include <stdio.h>
#include <uk/print.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following block is only enabled if supported backends are available.
 * TODO: In order to add support for another backend library, extend this
 *       #if-condition and implement a printing handler (_asmdump())
 *       in `asmdump.c`
 */
#if CONFIG_LIBZYDIS

#ifdef __IN_LIBUKDEBUG__
/*
 * This redefinition of CONFIG_LIBUKDEBUG_PRINTD is doing the trick to
 * switch on the correct declaration of uk_hexdumpd() when we are compiling
 * this library and have the global debug switch CONFIG_LIBUKDEBUG_PRINTD
 * not enabled.
 */
#if !defined CONFIG_LIBUKDEBUG_PRINTD || !CONFIG_LIBUKDEBUG_PRINTD
#undef CONFIG_LIBUKDEBUG_PRINTD
#define CONFIG_LIBUKDEBUG_PRINTD 1
#endif
#endif /* __IN_LIBUKDEBUG__ */

#if (defined UK_DEBUG) || CONFIG_LIBUKDEBUG_PRINTD
/* Please use uk_asmdumpd() instead */
void _uk_asmdumpd(const char *libname, const char *srcname,
		  unsigned int srcline, const void *instr,
		  unsigned int instr_count);

#define uk_asmdumpd(instr, instr_count)					\
	_uk_asmdumpd(__STR_LIBNAME__, __STR_BASENAME__,			\
		     __LINE__, (instr), (instr_count))
#else /* (defined UK_DEBUG) || CONFIG_LIBUKDEBUG_PRINTD */
static inline void uk_asmdumpd(const void *instr __unused,
			      unsigned int instr_count __unused)
{}
#endif

#if CONFIG_LIBUKDEBUG_PRINTK
/* Please use uk_asmdumpk() instead */
void _uk_asmdumpk(int lvl, const char *libname, const char *srcname,
		 unsigned int srcline, const void *instr,
		 unsigned int instr_count);

#define uk_asmdumpk(lvl, instr, instr_count)				\
	do {                                                            \
		if ((lvl) <= KLVL_MAX)                                  \
			_uk_asmdumpk((lvl), __STR_LIBNAME__, __STR_BASENAME__, \
				     __LINE__, (instr), (instr_count));	\
	} while (0)
#else /* CONFIG_LIBUKDEBUG_PRINTK */
static inline void uk_asmdumpk(int lvl __unused, const void *instr __unused,
			      unsigned int instr_count __unused)
{}
#endif /* CONFIG_LIBUKDEBUG_PRINTK */

#else /* Backends */
/*
 * In case there is no supported backend, we remove the asmdump(d|k)
 * calls from the code:
 */
static inline void uk_asmdumpd(const void *instr __unused,
			       unsigned int instr_count __unused)
{}

static inline void uk_asmdumpk(int lvl __unused, const void *instr __unused,
			       unsigned int instr_count __unused)
{}

#endif /* Backends */

#ifdef __cplusplus
}
#endif

#endif /* __UKDEBUG_ASMDUMP__ */
