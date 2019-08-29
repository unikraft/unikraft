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
extern const uk_ctor_func_t uk_ctortab[];
extern const uk_ctor_func_t uk_ctortab_end;

/**
 * Register a Unikraft constructor function that is
 * called during bootstrap (uk_ctortab)
 *
 * @param lvl
 *   Priority level (0 (higher) to 7 (least))
 *   Note: Any other value for level will be ignored
 * @param ctorf
 *   Constructor function to be called
 */
#define __UK_CTOR_FUNC(lvl, ctorf) \
		static const uk_ctor_func_t	\
		__used __section(".uk_ctortab" #lvl)	\
		__uk_ctab ## lvl ## _ ## ctorf = (ctorf)
#define UK_CTOR_FUNC(lvl, ctorf) __UK_CTOR_FUNC(lvl, ctorf)

/**
 * Helper macro for iterating over constructor pointer arrays
 * Please note that the array may contain NULL pointer entries
 *
 * @param arr_start
 *   Start address of pointer array (type: const uk_ctor_func_t const [])
 * @param arr_end
 *   End address of pointer array
 * @param i
 *   Iterator variable (integer) which should be used to access the
 *   individual fields
 */
#define uk_ctor_foreach(arr_start, arr_end, i)			\
	for ((i) = 0;						\
	     &((arr_start)[i]) < &(arr_end);			\
	     ++(i))

#ifdef __cplusplus
}
#endif

#endif /* __UK__CTORS_H__ */
