/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __HYPERLIGHT_TIME_H__
#define __HYPERLIGHT_TIME_H__

/**
 * Re-anchor the wall clock on the host's, after a snapshot restore: the
 * guest's monotonic clock (the TSC, which Hyperlight restores) carried
 * on from the moment of the snapshot, and the time the image spent on
 * disk, or the clock of a different host, is not in it.  Asks the host
 * (GetWallClockNs) and moves the epoch; the monotonic clock is untouched
 * so pending timers keep their meaning.
 */
void hyperlight_time_resync(void);
/* hyperlight_time_resync() with the host's wall clock (ns) in hand. */
void hyperlight_time_resync_to(__u64 now);

#if defined(__aarch64__)
/**
 * Carry the monotonic clock on from where it stood when the snapshot was
 * taken, after a restore.  The arm64 counter is not part of the restored
 * state (the TSC is), so a restored guest would otherwise see its clock
 * jump, possibly backwards.  Runs before anything reads the clock.
 */
void hyperlight_time_restore(void);
#endif

#endif /* __HYPERLIGHT_TIME_H__ */
