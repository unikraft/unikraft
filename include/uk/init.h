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
#include <uk/prio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*uk_init_func_t)(void);

/**
 * Register a Unikraft init function that is
 * called during bootstrap (uk_inittab)
 *
 * @param fn
 *   Initialization function to be called
 * @param class
 *   Initialization class (1 (earliest) to 6 (latest))
 * @param prio
 *   Priority level (0 (earliest) to 9 (latest)), must be a constant.
 *   Use the UK_PRIO_AFTER() helper macro for computing priority dependencies.
 *   Note: Any other value for level will be ignored
 */
#define __UK_INITTAB(fn, base, prio)					\
	static const uk_init_func_t					\
	__used __section(".uk_inittab" #base #prio)			\
	__uk_inittab ## base ## prio ## _ ## fn = (fn)

#define _UK_INITTAB(fn, base, prio)					\
	__UK_INITTAB(fn, base, prio)

#define uk_initcall_class_prio(fn, class, prio)				\
	_UK_INITTAB(fn, class, prio)

/**
 * Define a library initialization. At this point in time some platform
 * component may not be initialized, so it wise to initializes those component
 * to initialized.
 */
#define UK_INIT_CLASS_EARLY 1
#define uk_early_initcall_prio(fn, prio) \
	uk_initcall_class_prio(fn, UK_INIT_CLASS_EARLY, prio)
/**
 * Define a stage for platform initialization. Platform at this point read
 * all the device and device are initialized.
 */
#define UK_INIT_CLASS_PLAT 2
#define uk_plat_initcall_prio(fn, prio) \
	uk_initcall_class_prio(fn, UK_INIT_CLASS_PLAT, prio)
/**
 * Define a stage for performing library initialization. This library
 * initialization is performed after the platform is completely initialized.
 */
#define UK_INIT_CLASS_LIB 3
#define uk_lib_initcall_prio(fn, prio) \
	uk_initcall_class_prio(fn, UK_INIT_CLASS_LIB, prio)
/**
 * Define a stage for filesystem initialization.
 */
#define UK_INIT_CLASS_ROOTFS 4
#define uk_rootfs_initcall_prio(fn, prio) \
	uk_initcall_class_prio(fn, UK_INIT_CLASS_ROOTFS, prio)
/**
 * Define a stage for device initialization
 */
#define UK_INIT_CLASS_SYS 5
#define uk_sys_initcall_prio(fn, prio) \
	uk_initcall_class_prio(fn, UK_INIT_CLASS_SYS, prio)
/**
 * Define a stage for application pre-initialization
 */
#define UK_INIT_CLASS_LATE 6
#define uk_late_initcall_prio(fn, prio) \
	uk_initcall_class_prio(fn, UK_INIT_CLASS_LATE, prio)

/**
 * Similar interface without priority.
 */
#define uk_early_initcall(fn)     uk_early_initcall_prio(fn, UK_PRIO_LATEST)
#define uk_plat_initcall(fn)      uk_plat_initcall_prio(fn, UK_PRIO_LATEST)
#define uk_lib_initcall(fn)       uk_lib_initcall_prio(fn, UK_PRIO_LATEST)
#define uk_rootfs_initcall(fn)    uk_rootfs_initcall_prio(fn, UK_PRIO_LATEST)
#define uk_sys_initcall(fn)       uk_sys_initcall_prio(fn, UK_PRIO_LATEST)
#define uk_late_initcall(fn)      uk_late_initcall_prio(fn, UK_PRIO_LATEST)

extern const uk_init_func_t uk_inittab_start[];
extern const uk_init_func_t uk_inittab_end;

/**
 * Helper macro for iterating over init pointer tables
 * Please note that the table may contain NULL pointer entries
 *
 * @param itr
 *   Iterator variable (uk_init_func_t *) which points to the individual
 *   table entries during iteration
 * @param inittab_start
 *   Start address of table (type: const uk_init_func_t[])
 * @param inittab_end
 *   End address of table (type: const uk_init_func_t)
 */
#define uk_inittab_foreach(itr, inittab_start, inittab_end)	\
	for ((itr) = DECONST(uk_init_func_t*, inittab_start);	\
	     (itr) < &(inittab_end);				\
	     (itr)++)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _UK_INIT_H */
