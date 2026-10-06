/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __HYPERLIGHT_CONSOLE_H__
#define __HYPERLIGHT_CONSOLE_H__

/**
 * Forget which console host functions the host lacked: after a snapshot
 * restore, the host is another, and is asked afresh.
 */
void hyperlight_console_resume(void);

#endif /* __HYPERLIGHT_CONSOLE_H__ */
