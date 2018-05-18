/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __UKPLAT_CTORS_H__
#define __UKPLAT_CTORS_H__

#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ukplat_ctor_func_t)(void);

/* Function pointer arrays of constructors; provided by
 * the platform's linker script */
extern const ukplat_ctor_func_t const __preinit_array_start[];
extern const ukplat_ctor_func_t const __preinit_array_end;
extern const ukplat_ctor_func_t const __init_array_start[];
extern const ukplat_ctor_func_t const __init_array_end;

/**
 * Helper macro for iterating over constructor pointer arrays
 * Please note that the array may contain NULL pointer entries
 *
 * @param arr_start
 *   Start address of pointer array (type: const ukplat_ctor_func_t const [])
 * @param arr_end
 *   End address of pointer array
 * @param i
 *   Iterator variable (integer) which should be used to access the
 *   individual fields
 */
#define ukplat_ctor_foreach(arr_start, arr_end, i)			   \
	for ((i)=0;							   \
	     &((arr_start)[i]) < &(arr_end); \
	     ++(i))

#ifdef __cplusplus
}
#endif

#endif /* __UKPLAT_CTORS_H__ */
