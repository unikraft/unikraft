/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight guest dispatch API.
 *
 * After evolve, the host can invoke guest functions by pushing a
 * FunctionCall FlatBuffer onto the PEB input stack and setting RIP
 * to hyperlight_dispatch_function.  The dispatch handler pops the
 * call, pushes a void result, and halts via port 108.
 */

#ifndef __HYPERLIGHT_DISPATCH_H__
#define __HYPERLIGHT_DISPATCH_H__

#include <uk/arch/types.h>

struct hyperlight_peb;

/**
 * Initialise the dispatch subsystem.
 *
 * Caches PEB I/O stack pointers and queries the host for the scratch
 * exception stack top via GetExnStackTop.  Must be called during boot
 * after hl_hcall_init().
 */
void hyperlight_dispatch_init(const struct hyperlight_peb *peb);

/**
 * Largest FunctionCall the host can send: the PEB input stack size, read
 * at init.  Every guest-side buffer that holds a call is sized from it.
 */
__u64 hyperlight_dispatch_max_call(void);

/**
 * Dispatch entry point for guest function calls.
 *
 * Set as RIP by the host via dispatch_call_from_host.  Never returns
 * — halts the vCPU after pushing the result.
 */
void hyperlight_dispatch_function(void) __attribute__((noreturn));

#endif /* __HYPERLIGHT_DISPATCH_H__ */
