/* SPDX-License-Identifier: MIT */
/******************************************************************************
 * arch-x86_64.h
 *
 * Guest OS interface to x86 64-bit Xen.
 *
 * Copyright (c) 2004-2006, K A Fraser
 */

#include "arch-x86/xen.h"

/*
 * ` enum neg_errnoval
 * ` HYPERVISOR_set_callbacks(unsigned long event_selector,
 * `                          unsigned long event_address,
 * `                          unsigned long failsafe_selector,
 * `                          unsigned long failsafe_address);
 * `
 * Register for callbacks on events.  When an event (from an event
 * channel) occurs, event_address is used as the value of eip.
 *
 * A similar callback occurs if the segment selectors are invalid.
 * failsafe_address is used as the value of eip.
 *
 * On x86_64, event_selector and failsafe_selector are ignored (?!?).
 */
