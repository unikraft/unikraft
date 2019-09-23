/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
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
#ifndef _UK_INIT_H
#define _UK_INIT_H

#include <uk/config.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*uk_init_t)(void);

#define INITTAB_STR_VAR(libname, fn, base, prio) libname ## fn ## base ## prio
#define INITTAB_SECTION(base, prio)  .uk_inittab_ ## base ## prio
#define INITTAB_SECTION_NAME(name) STRINGIFY(name)

#define __inittab(libname, fn, base, prio)				\
	static  const __used __section(INITTAB_SECTION_NAME(		\
					INITTAB_SECTION(base, prio))	\
				      )					\
		uk_init_t INITTAB_STR_VAR(libname, fn, base, prio) = fn

/**
 * Define a library initialization. At this point in time some platform
 * component may not be initialized, so it wise to initializes those component
 * to initialized.
 */
#define uk_early_initcall_prio(fn, prio)  __inittab(LIBNAME, fn, 1, prio)
/**
 * Define a stage for platform initialization. Platform at this point read
 * all the device and device are initialized.
 */
#define uk_plat_initcall_prio(fn, prio)  __inittab(LIBNAME, fn, 2, prio)
/**
 * Define a stage for performing library initialization. This library
 * initialization is performed after the platform is completely initialized.
 */
#define uk_lib_initcall_prio(fn, prio)	__inittab(LIBNAME, fn, 3, prio)
/**
 * Define a stage for filesystem initialization.
 */
#define uk_rootfs_initcall_prio(fn, prio) __inittab(LIBNAME, fn, 4, prio)
/**
 * Define a stage for device initialization
 */
#define uk_sys_initcall_prio(fn, prio) __inittab(LIBNAME, fn, 5, prio)
/**
 * Define a stage for application pre-initialization
 */
#define uk_late_initcall_prio(fn, prio)  __inittab(LIBNAME, fn, 6, prio)

/**
 * Similar interface without priority.
 */
#define uk_early_initcall(fn)     uk_early_initcall_prio(fn, 9)
#define uk_plat_initcall(fn)      uk_plat_initcall_prio(fn, 9)
#define uk_lib_initcall(fn)       uk_lib_initcall_prio(fn, 9)
#define uk_rootfs_initcall(fn)    uk_rootfs_initcall_prio(fn, 9)
#define uk_sys_initcall(fn)       uk_sys_initcall_prio(fn, 9)
#define uk_late_initcall(fn)      uk_late_initcall_prio(fn, 9)

extern const uk_init_t uk_inittab_start[];
extern const uk_init_t uk_inittab_end;

#define uk_inittab_foreach(init_start, init_end, itr)		\
	for (itr = DECONST(uk_init_t*, init_start); itr < &init_end; itr++)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _UK_INIT_H */
