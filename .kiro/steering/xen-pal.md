---
inclusion: fileMatch
fileMatchPattern: 'plat/xen/pal/**/*,Test_Specifications/xen/pal/**/*.md'
---

# Xen PAL — directory-level steering

## Purpose and scope

Use this guidance when reviewing any header under `plat/xen/pal/` or drafting/revising specifications under `Test_Specifications/xen/pal/` in this Unikraft fork. Follow `.kiro/steering/xen-test-specifications.md` as the primary project-wide rules. This file adds PAL-wide review prompts; the topic files `xen-pal-except.md`, `xen-pal-context.md`, and `xen-pal-memory.md` add header-specific detail. None of these replace the project-wide rules.

Produce **source-derived, reviewable draft requirements and proposed tests**, not approved safety artifacts. Modify specification documents only unless the user separately requests source changes. Do not claim that any test has been executed without actual execution evidence.

Cover the common headers and the `x86_64` arch headers. The `arm64` arch headers are out of scope unless the user asks; do not transfer arm64 behavior into an x86_64 requirement or the reverse.

## Inspect source and dependencies first

1. List the **current checked-out contents** of `plat/xen/pal/` (for example with `find plat/xen/pal -type f`). The directory contains headers only: common headers in `plat/xen/pal/include/uk/plat/pal/` and arch headers in `plat/xen/pal/arch/<arch>/include/uk/plat/pal/arch/`. Do not rely on a remembered file list.
2. Inspect the backends each header forwards to: Xen headers in `plat/xen/include/uk/plat/xen/` and `plat/xen/arch/x86_64/include/uk/plat/xen/arch/`, and native headers in `plat/native/include/uk/plat/native/`. Follow them only far enough to tell a wrapper fact from backend behavior.
3. Inspect the generic PAL contract in `lib/ukpal/include/uk/pal/` and `lib/ukpal/arch/x86_64/include/uk/pal/arch/`, and the consumers in `lib/uklcpu/include/uk/lcpu/` and `lib/ukpaging/`. Find any other consumers with `grep -rln "uk/plat/pal/" --include=*.h --include=*.c .`.
4. Inspect the "PAL includes" block in `plat/xen/Makefile.uk`. Confirm that `pal/include` and `pal/arch/$(CONFIG_UK_ARCH)/include` are added to the AS, C, and C++ include paths under `CONFIG_PLAT_XEN`, and state which items are conditional on that.
5. Use the **actual code** as evidence. Do not infer guaranteed behavior from macro names, header comments, or intended platform architecture alone.

## PAL-wide behavior to investigate

Verify each item against the current source before turning it into a requirement:

- **Header resolution**: `plat/native/pal/` contains headers with the same names and include paths. Determine which set a Xen build resolves for `<uk/plat/pal/...>`, including include-path order. Never cite native PAL files as evidence for Xen.
- **Forwarding classification**: for every macro and inline function, record whether it maps to a `UK_PLAT_XEN_*` / `uk_plat_xen_*` name (Xen-forwarded), a `UK_PLAT_NATIVE_*` / `uk_plat_native_*` name (native-forwarded), a mix of both within one header, or is deliberately absent (stated by a comment or an empty header).
- **Include guards**: each common header defines `__UK_PLAT_PAL_<NAME>_H__`; each arch header raises `#error "Do not include this header directly"` unless its parent guard is defined.
- **Assembly visibility**: which items sit inside `#if !__ASSEMBLY__` and which are visible to assembly, plus `extern "C"` wrapping.
- **`__isr` annotations**: which inline wrappers carry `__isr` and which do not. Report as observed; do not explain the reason without evidence.
- **Pointer casts**: wrappers cast opaque `struct uk_pal_*` pointers to backend struct types. The wrapper passes the pointer through; layout compatibility is a backend contract and must be listed as an assumption.
- **Type differences**: any mismatch between a wrapper and its backend, or between a paired getter and setter. Report what the code declares; raise it as an open question, not a defect, unless a consequence is shown.

**Do not assume** that a forwarding wrapper establishes the behavior of the function it forwards to. Interrupt masking, context switching, TLB flushing, and similar effects are backend behavior that requires a separate specification or an explicit assumption.

## Requirements drafting rules

Write one specification per common header, `Test_Specifications/xen/pal/<header>.md`, and cover the matching x86_64 arch header in its own section of the same document. Use the same layout as the team's `arch_time.md` and `arch_events.md` examples:

1. Title and explicit **draft/example — not approved safety artifact** notice.
2. Source scope, the backend headers relied on, and relevant configuration.
3. Concise source observations, including the forwarding classification for each item.
4. Requirements with stable unique IDs, `REQ-PAL-<TOPIC>-NNN` for common headers and `REQ-PAL-X86-<TOPIC>-NNN` for x86_64 arch headers (`<TOPIC>` is the header name in capitals, e.g. `EXCEPT`, `REGS`, `SYSCTX`), each with a triggering condition/precondition, one observable obligation using **shall**, and a trace to an exact repository-relative path and macro or function.
5. Proposed tests with stable unique IDs, `TEST-PAL-<TOPIC>-NNN` and `TEST-PAL-X86-<TOPIC>-NNN`, links to the requirements they cover, test method, preconditions, ordered steps, and explicit measurable expected results.
6. Assumptions (including backend behavior), prerequisites, open questions, and coverage gaps.

Typical obligations are macro forwarding ("`UK_PAL_X` shall expand to `UK_PLAT_XEN_X`"), function forwarding ("shall call `uk_plat_xen_f` once with the same argument values and return its result unchanged"), deliberate absence ("shall not declare `f`", only where the header itself shows it), and include protection. Closely related macros, such as a block of register offsets, may share one requirement with a mapping table if one test checks all of them. Preserve existing IDs, check for duplicates across `Test_Specifications/xen/`, and do not invent requirements for behavior the headers do not establish.

## Testing and evidence

- Select an appropriate method for each claim: source inspection, preprocessor expansion check (`-E` with Xen include paths), compile-time assertion (`_Static_assert` comparing `UK_PAL_*` with backend values), isolated unit test with stub backend functions, or Xen-configured build/link check. Explain what the chosen method actually proves.
- A stub-backend unit test proves forwarding, argument pass-through, and return pass-through. It does not prove backend behavior. A build/link check proves the headers compile together; it does not prove guest runtime behavior.
- **Do not invent** an existing harness, executable commands, stubs, results, or infrastructure. Describe any harness as proposed and list the include paths and configuration it would need.
- Use distinctive argument and return values in forwarding tests so a dropped, truncated, or swapped value is visible.
- Label all unexecuted tests **Proposed — not run**. If tests are eventually run, record the command, configuration, environment, result, and evidence location.
- Ensure each requirement has at least one proposed test and each test identifies its covered requirement(s).

## Review checklist

Before finishing, verify:

- Every macro and inline function in scope is covered by a requirement or listed as a coverage gap.
- Each requirement names the backend (Xen or native) it forwards to, as observed in the current header.
- Source paths and symbol names are exact; any line numbers have been rechecked.
- Backend behavior is stated as an assumption, not claimed from the wrapper, and native PAL files were not used as evidence.
- Requirement and test IDs are unique and cross-referenced.
- Proposed tests are not presented as executed or safety-approved.

Summarize what each PAL specification covers, what remains unverified, and any discrepancies between the documents and the current headers. Do not change implementation code as part of a specification-only task.
