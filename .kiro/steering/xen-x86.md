---
inclusion: fileMatch
fileMatchPattern: ["Test_Specifications/xen/x86/**/*.md", "plat/xen/x86/**/*"]
---

# Xen x86 specification guidance

## Starting scope

The first specification folder is `Test_Specifications/xen/x86/`. Its existing
examples cover event lifecycle hooks and timer lifecycle operations; they do
not yet cover every function in the corresponding C files.

Read these files when working on that scope:

- `plat/xen/x86/arch_events.c`: architecture event hooks.
- `plat/xen/events.c`: common initialization, cleanup, and binding behavior.
- `plat/xen/include/common/events.h`: event interface declarations.
- `plat/xen/x86/arch_time.c`: clock reads, timer lifecycle, and blocking.
- `plat/xen/Makefile.uk` and `plat/xen/Config.uk`: build/configuration context.

The Makefile selects these x86 C files with `CONFIG_ARCH_X86_64`. Do not describe
the folder as evidence of x86-32 support, and do not transfer ARM behavior into
an x86 requirement.

## Event lifecycle

- `arch_init_events`, `arch_unbind_ports`, and `arch_fini_events` currently have
  empty bodies. Describe them as hooks with no state-changing statements in
  this implementation, rather than inventing architecture-specific work.
- `init_events` installs the default handler and masks each common event entry
  before calling `arch_init_events`.
- `fini_events` calls `arch_unbind_ports`, then `unbind_all_ports`, then
  `arch_fini_events`. Preserve this order in requirements and test oracles.
- Inspection can demonstrate empty bodies and call order; a configured
  build/link check can demonstrate symbol resolution. Neither alone proves
  event delivery inside a Xen guest.

## Time and timer behavior

- `ukplat_time_init` binds `VIRQ_TIMER` with `timer_handler` and `NULL` data,
  stores the returned port, and unmasks that port. Its body does not check for
  bind failure. Treat successful binding as a precondition for the existing
  normal-path example; record failure handling as an open question rather than
  claiming a recovery path.
- `timer_handler` requests an expiry equal to the monotonic-clock value it
  obtains plus `UKPLAT_TIME_TICK_NSEC`. A deterministic proposed test should
  control that clock return value and capture the timer hypercall argument.
  A separate clock sample taken before handler entry is not an exact oracle.
  Timer programming does not prove exact interrupt arrival time or jitter.
- `ukplat_time_fini` requests timer value `0` before unbinding the stored port.
  Do not infer repeated-finalization safety from this sequence.
- For a next increment, inspect `time_block_until`: it asserts interrupts are
  disabled, and only for a future deadline does it program the timer, halt with
  interrupts, then clear the timer. Propose both future and equal/past deadline
  cases. Do not claim it loops until the deadline or cannot wake early.
- Clock conversion and version-retry behavior require their own analysis of
  `scale_delta`, `get_time_values_from_xen`, `ukplat_monotonic_clock`,
  `update_wallclock`, and `ukplat_wall_clock`. The timer examples do not cover
  arithmetic, shared-memory consistency, or clock accuracy.
- Migration helpers are guarded by `CONFIG_MIGRATION` and marked TODO in the
  implementation. Keep any migration specification separate and conditional;
  do not claim migration was built or tested without evidence.

## Completion criteria for this folder

Keep the existing `TIME` and `EVENT` ID families. For each addition, trace the
specific branch or call sequence, describe an observable test result, and list
any required harness work. Leave runtime validation explicitly pending unless
an appropriate configured test was actually run.
