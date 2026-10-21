/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __HYPERLIGHT_CONSOLE_H__
#define __HYPERLIGHT_CONSOLE_H__

#include <uk/arch/types.h>

/**
 * Forget which console host functions the host lacked: after a snapshot
 * restore, the host is another, and is asked afresh.
 */
void hyperlight_console_resume(void);

/**
 * Take the guest log level the host gave at entry (a GuestLogFilter: 0
 * off to 5 trace; 1, errors, for one out of range): kernel messages less
 * severe than it are not sent to the host's log.
 */
void hyperlight_console_log_init(__u64 max_log_level);

#endif /* __HYPERLIGHT_CONSOLE_H__ */
