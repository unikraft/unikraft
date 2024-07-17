/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_BOOT_BOOT_H__
#define __UK_BOOT_BOOT_H__

#include <uk/config.h>
#include <errno.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/common/bootinfo.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Executes the early_init boot stage
 *
 * Once platform code is updated to register
 * via earlytab, this function will be called
 * directly by uk_boot_entry()
 *
 * @param bi pointer to the bootinfo structure
 */
void uk_boot_early_init(struct ukplat_bootinfo *bi);

/**
 * Requests the "init" thread to perform a shutdown
 *
 * @param target
 *   The action that shall be performed after shutdown:
 *   - `UKPLAT_HALT`: Terminate/Halt/Power-off
 *   - `UKPLAT_RESTART`: Restart/Reboot
 *   - `UKPLAT_CRASH`: Indicate a crash
 * @returns
 *   - (0): Success, shutdown is requested.
 *   - (1): A shutdown is already in progress, note that `request` is
 *          ignored.
 *   - (-ENOTSUP): Shutdown requests are not supported.
 *   - (-EINVAL): Unsupported shutdown `target`.
 */
#if CONFIG_LIBUKBOOT_MAINTHREAD
int uk_boot_shutdown_req(enum ukplat_gstate target);
#else /* !CONFIG_LIBUKBOOT_MAINTHREAD */
#define uk_boot_shutdown_req(target) \
	({ -ENOTSUP; })
#endif /* !CONFIG_LIBUKBOOT_MAINTHREAD */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_BOOT_BOOT_H__ */
