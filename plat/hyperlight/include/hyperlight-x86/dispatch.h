/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __HYPERLIGHT_X86_DISPATCH_H__
#define __HYPERLIGHT_X86_DISPATCH_H__

#include <hyperlight-x86/peb.h>

/**
 * Callback type for the application runner.
 * The elfloader registers a function that dispatch invokes to run the ELF.
 */
typedef void (*hl_run_fn)(void);

/**
 * Initialize the dispatch subsystem with a copy of the PEB.
 * Must be called during kernel init (PEB lives in read-only snapshot memory).
 */
void hyperlight_dispatch_init(const struct hyperlight_peb *peb);

/**
 * Register the callback that dispatch invokes to run the application.
 * Called by the elfloader after loading the ELF but before signaling ready.
 */
void hyperlight_dispatch_register(hl_run_fn fn);

/**
 * The dispatch function entry point.
 * Address is returned to the host in RAX during init so the host can
 * set RIP here for each MultiUseSandbox::call().
 *
 * Never returns.
 */
void hyperlight_dispatch_function(void) __attribute__((noreturn));

#endif /* __HYPERLIGHT_X86_DISPATCH_H__ */
