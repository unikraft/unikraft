/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __HYPERLIGHT_RESOLV_H__
#define __HYPERLIGHT_RESOLV_H__

/**
 * Fetch the host's resolver configuration (GetResolvConf) and write it
 * as the guest's /etc/resolv.conf.  A late initcall runs it at boot,
 * once the rootfs is mounted; hl_resume() runs it again after a restore
 * from a snapshot.  An empty answer, or no such host function, leaves
 * the image's own file alone.
 */
void hyperlight_resolv_apply(void);

#endif /* __HYPERLIGHT_RESOLV_H__ */
