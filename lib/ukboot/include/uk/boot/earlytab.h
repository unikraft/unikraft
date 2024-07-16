/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_BOOT_EARLYTAB_H__
#define __UK_BOOT_EARLYTAB_H__

#include <uk/config.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/common/common.lds.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*earlytab_func_t)(struct ukplat_bootinfo *bi);

struct uk_boot_earlytab_entry {
	earlytab_func_t init;
};

extern const struct uk_boot_earlytab_entry uk_boot_earlytab_start[];
extern const struct uk_boot_earlytab_entry uk_boot_earlytab_end;

#define _UK_EARLYTAB_ENTRY(init_fn, prio)				\
	static const struct uk_boot_earlytab_entry			\
	__used __section(".uk_boot_earlytab" #prio) __align(8)		\
	__uk_boot_earlytab ## prio ## _ ## init_fn = {			\
		.init = (init_fn),					\
	}

/**
 * Registration to earlytab
 *
 * Libraries that need to execute code at early_init
 * shall register this macro.
 *
 * @param init_fn early init function to register. For
 *                the prototype see @earlytab_func_t
 * @param prio	  priority to execute the function at
 */
#define UK_BOOT_EARLYTAB_ENTRY(init_fn, prio)				\
	_UK_EARLYTAB_ENTRY(init_fn, prio)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UK_BOOT_EARLYTAB_H__ */
