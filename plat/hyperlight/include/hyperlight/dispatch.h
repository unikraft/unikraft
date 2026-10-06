/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight guest dispatch API.
 *
 * After evolve, the host can invoke guest functions by writing a
 * FunctionCall FlatBuffer into the H2G virtqueue and setting RIP to
 * hyperlight_dispatch_function.  The dispatch handler takes the call,
 * hands it to the cooperative step pump (step.h), queues a void result
 * on G2H, and halts via port 108.
 */

#ifndef __HYPERLIGHT_DISPATCH_H__
#define __HYPERLIGHT_DISPATCH_H__

#include <uk/arch/types.h>

/* The least a size-prefixed FunctionCall takes: its size prefix and the
 * offset of its root table.
 */
#define HL_FC_MIN_LEN 8

/**
 * Initialise the dispatch subsystem.
 *
 * Queries the host for the scratch exception stack top via
 * GetExnStackTop.  Must be called during boot after hl_hcall_init().
 */
void hyperlight_dispatch_init(void);

/**
 * Largest FunctionCall the host can send: what the H2G receive buffers
 * hold together.  Every guest-side buffer that holds a call is sized
 * from it.
 */
__u64 hyperlight_dispatch_max_call(void);

/**
 * Dispatch entry point for guest function calls.
 *
 * Set as RIP by the host via dispatch_call_from_host.  Never returns
 * — halts the vCPU after pushing the result.
 */
void hyperlight_dispatch_function(void) __attribute__((noreturn));

/**
 * Answer the guest function call in flight, if any, with a void result,
 * once.
 */
void hyperlight_dispatch_push_void_result(void);

/**
 * End boot: ready the transport for the boot snapshot (see
 * hl_transport_checkpoint()) and halt to the host (shutdown.c).
 */
void hyperlight_halt_boot_done(void) __attribute__((noreturn));

/**
 * Whether boot has ended: set just before the boot halt, after its
 * checkpoint; later entries are calls or snapshot checkpoints.
 */
int hyperlight_booted(void);

#endif /* __HYPERLIGHT_DISPATCH_H__ */
