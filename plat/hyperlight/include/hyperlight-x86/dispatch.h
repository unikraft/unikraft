/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __HYPERLIGHT_X86_DISPATCH_H__
#define __HYPERLIGHT_X86_DISPATCH_H__

#include <uk/arch/types.h>
#include <hyperlight-x86/peb.h>

/**
 * Callback type for the application runner.
 * The elfloader registers a function that dispatch invokes to run the ELF.
 */
typedef void (*hl_run_fn)(void);

/**
 * FC-aware dispatch callback. Receives the raw FunctionCall FlatBuffer
 * bytes popped from the input stack — the callback pulls its own
 * function_name + parameters out with the hl_fb_* helpers in fb.h and
 * does its own per-function routing.
 */
typedef void (*hl_dispatch_fn)(const __u8 *fc_bytes, __sz fc_len);

/**
 * Initialize the dispatch subsystem with a copy of the PEB.
 * Must be called during kernel init (PEB lives in read-only snapshot memory).
 */
void hyperlight_dispatch_init(const struct hyperlight_peb *peb);

/**
 * Register the legacy no-args callback that dispatch invokes to run
 * the application. Called by the elfloader after loading the ELF but
 * before signaling ready. Used when the ELF exposes a classic
 * main()/_start and doesn't participate in multi-function routing.
 */
void hyperlight_dispatch_register(hl_run_fn fn);

/**
 * Register an FC-aware dispatch callback. If set, `hyperlight_dispatch_inner`
 * peeks the FunctionCall FlatBuffer off the input stack, pops the
 * stack, and invokes this callback with the raw bytes. The ELF does
 * its own name-based routing inside the callback.
 *
 * Takes precedence over `hyperlight_dispatch_register` when both are set.
 */
void hyperlight_dispatch_register_v2(hl_dispatch_fn fn);

/**
 * The dispatch function entry point.
 * Address is returned to the host in RAX during init so the host can
 * set RIP here for each MultiUseSandbox::call().
 *
 * Never returns.
 */
void hyperlight_dispatch_function(void) __attribute__((noreturn));

/**
 * Current in-flight FunctionCall bytes. Populated by dispatch_inner
 * immediately before a callback runs; valid until the callback returns.
 *
 * These are reachable from in-tree kernel-linked code. Separately-loaded
 * ELFs (dynamically-linked drivers loaded by app-elfloader) can read the
 * live values by stashing the slot *addresses* into the ELF's environ
 * at boot and dereferencing them at dispatch time — see the slot
 * accessors below.
 */
const __u8 *hyperlight_dispatch_current_fc_bytes(void);
__sz hyperlight_dispatch_current_fc_len(void);

/**
 * Addresses of the current-FC globals. app-elfloader grabs these at
 * boot and hands them to the loaded ELF via env vars so driver code
 * can read the in-flight bytes without linking against kernel symbols.
 */
const __u8 **hyperlight_dispatch_fc_bytes_slot(void);
__sz *hyperlight_dispatch_fc_len_slot(void);

/**
 * Address of the FC-aware dispatch callback pointer. A user-mode
 * driver writes its own function into `*slot` during its one-time
 * init (from the initial deferred_run / main() dispatch). Every
 * subsequent call goes through that callback, bypassing the legacy
 * deferred-main path entirely.
 */
hl_dispatch_fn *hyperlight_dispatch_v2_slot(void);

/**
 * Per-syscall count + cumulative-ns profiler. Populated via
 * syscall_shim enter/exit hooks.
 *
 * Call `dump` to print the top syscalls by total time to stderr, and
 * `reset` to clear the counters (useful to isolate call-phase cost
 * from evolve-time noise).
 */
void hyperlight_syscall_profile_dump(void);
void hyperlight_syscall_profile_reset(void);

#endif /* __HYPERLIGHT_X86_DISPATCH_H__ */
