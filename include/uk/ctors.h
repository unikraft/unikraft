/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *			Vlad-Andrei Badoiu <vlad_andrei.badoiu@stud.acs.upb.ro>
 *
 *
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __UK_CTORS_H__
#define __UK_CTORS_H__

#include <uk/essentials.h>
#include <uk/prio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*uk_ctor_func_t)(void);

/*
 * Function pointer arrays of constructors; provided by
 * the platform's linker script
 */
extern const uk_ctor_func_t __preinit_array_start[];
extern const uk_ctor_func_t __preinit_array_end;
extern const uk_ctor_func_t __init_array_start[];
extern const uk_ctor_func_t __init_array_end;
extern const uk_ctor_func_t uk_ctortab_start[];
extern const uk_ctor_func_t uk_ctortab_end;

/**
 * Register a Unikraft constructor function that is
 * called during bootstrap (uk_ctortab)
 *
 * @param fn
 *   Constructor function to be called
 * @param prio
 *   Priority level (0 (earliest) to 9 (latest))
 *   Use the UK_PRIO_AFTER() helper macro for computing priority dependencies.
 *   Note: Any other value for level will be ignored
 */
#define __UK_CTORTAB(fn, prio)				\
	static const uk_ctor_func_t			\
	__used __section(".uk_ctortab" #prio)		\
	__uk_ctortab ## prio ## _ ## fn = (fn)

#define _UK_CTORTAB(fn, prio)				\
	__UK_CTORTAB(fn, prio)

#define UK_CTOR_PRIO(fn, prio)				\
	_UK_CTORTAB(fn, prio)

/**
 * Similar interface without priority.
 */
#define UK_CTOR(fn) UK_CTOR_PRIO(fn, UK_PRIO_LATEST)

/* DELETEME: Compatibility wrapper for existing code, to be removed! */
#define UK_CTOR_FUNC(lvl, ctorf) \
	_UK_CTORTAB(ctorf, lvl)

/**
 * Helper macro for iterating over constructor pointer tables
 * Please note that the table may contain NULL pointer entries
 *
 * @param itr
 *   Iterator variable (uk_ctor_func_t *) which points to the individual
 *   table entries during iteration
 * @param ctortab_start
 *   Start address of table (type: const uk_ctor_func_t[])
 * @param ctortab_end
 *   End address of table (type: const uk_ctor_func_t)
 */
#define uk_ctortab_foreach(itr, ctortab_start, ctortab_end)	\
	for ((itr) = DECONST(uk_ctor_func_t*, ctortab_start);	\
	     (itr) < &(ctortab_end);				\
	     (itr)++)

#ifdef __cplusplus
}
#endif

#endif /* __UK__CTORS_H__ */
