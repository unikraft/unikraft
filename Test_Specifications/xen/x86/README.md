# Xen x86 capstone starting point

This folder is the first scope for the capstone's Kiro-assisted test
specifications. The existing `arch_events.md` and `arch_time.md` are example
specifications, not executed tests or approved safety artifacts.

## Using the steering

Open the **Unikraft repository root** in Kiro. The steering files are stored in
the workspace-level `.kiro/steering/` directory:

- `xen-test-specifications.md` sets evidence, traceability, and test-writing rules.
- `xen-x86.md` adds implementation-specific guidance for this folder.

Both use file-match inclusion for their relevant source/specification paths.
The empty `Test_Specifications/.kiro/steering/` directory is not the steering
location for this workspace setup.

## Suggested first task in Kiro

> Read `.kiro/steering/xen-test-specifications.md` and
> `.kiro/steering/xen-x86.md`. Review
> `Test_Specifications/xen/x86/arch_time.md` against
> `plat/xen/x86/arch_time.c`, then extend the specification with requirements
> and proposed tests for `time_block_until`. Cover future, equal, and past
> deadlines, the interrupt-disabled precondition, and the ordering of timer
> programming, halt, and timer cancellation. Preserve existing IDs, cite source
> symbols, identify harness prerequisites, and label tests as not run. Edit
> specification documents only.

## Current coverage

| Document | Existing example coverage | Work still needed |
| --- | --- | --- |
| `arch_events.md` | Empty x86 hooks and common lifecycle call order | Configured build/link validation |
| `arch_time.md` | Timer binding, rescheduling, and finalization | Blocking branches, clock arithmetic/version handling, failure assumptions, runtime validation |

Steering syntax reference: [Kiro steering documentation](https://kiro.dev/docs/steering/).
