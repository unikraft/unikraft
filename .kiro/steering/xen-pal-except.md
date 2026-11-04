---
inclusion: fileMatch
fileMatchPattern: 'plat/xen/pal/include/uk/plat/pal/except.h,plat/xen/pal/arch/x86_64/include/uk/plat/pal/arch/except.h,Test_Specifications/xen/pal/except.md'
---

# Xen PAL exceptions and interrupts — source-specific steering

## Purpose and scope

Use this guidance when reviewing `plat/xen/pal/include/uk/plat/pal/except.h` and `plat/xen/pal/arch/x86_64/include/uk/plat/pal/arch/except.h`, or drafting/revising `Test_Specifications/xen/pal/except.md` in this Unikraft fork. Follow `.kiro/steering/xen-test-specifications.md` and `.kiro/steering/xen-pal.md` as the primary rules. This file adds exception-specific review prompts; it does not replace those rules.

Produce **source-derived, reviewable draft requirements and proposed tests**, not approved safety artifacts. Modify specification documents only unless the user separately requests source changes. Do not claim that any test has been executed without actual execution evidence.

## Inspect source and dependencies first

1. Read the **current checked-out versions** of both PAL headers in full. Confirm that an existing `except.md` draft, if any, still matches them; do not assume earlier code or line numbers remain accurate.
2. Inspect the Xen backend declarations they forward to: `plat/xen/include/uk/plat/xen/except.h` and `plat/xen/arch/x86_64/include/uk/plat/xen/arch/except.h`. Locate the backend implementations (for example by searching for `uk_plat_xen_enable_irq` and `uk_plat_xen_except_init`) only to separate wrapper facts from backend behavior.
3. Inspect the consumer `lib/uklcpu/include/uk/lcpu/except.h` and any matching contract header under `lib/ukpal/`; confirm it exists before citing it.
4. Use the **actual code** as evidence. Do not infer guaranteed behavior from function names, comments, or intended platform architecture alone.

## Exception-specific behavior to investigate

Verify each item against the current source before turning it into a requirement:

- `UK_PAL_EXCEPT_EVENT_*` (DEBUG, ERR_INVALID_OP, ERR_PAGE_FAULT, ERR_BUS_ERROR, ERR_MATH, ERR_SECURITY, SYSCALL, IRQ, UNHANDLED): each maps to the same-named `UK_PLAT_XEN_EXCEPT_EVENT_*` macro. Note what the Xen backend maps them to in turn.
- `UK_PAL_X86_64_EXCEPT_EVENT_ERR_GP_FAULT` and `UK_PAL_X86_64_EXCEPT_EVENT_NMI`: map to `UK_PLAT_XEN_X86_64_*` macros and are defined outside `#if !__ASSEMBLY__`.
- `uk_pal_except_err_ctx_get/set_str`, `_handler_err`, `_regs`, `_fault_addr`: each forwards once to the matching `uk_plat_xen_*` accessor. Check the casts, including `set_regs` passing `struct uk_pal_regs *` as `struct uk_plat_native_regs *` to a Xen function.
- `uk_pal_x86_64_except_err_ctx_get/set_trapnr` and `_error_code`: forward to the x86_64 Xen accessors and are marked `__isr`, unlike the common accessors.
- `uk_pal_except_irq_ctx_get/set_regs` and `get/set_irq`: record that `get_irq` returns `__u64` while `set_irq` takes `__u32`, matching the backend declarations.
- `uk_pal_enable_irq`, `uk_pal_disable_irq`, `uk_pal_save_irqf`, `uk_pal_restore_irqf`, `uk_pal_irqs_disabled`, `uk_pal_irqs_handle_pending`: forwarding only. Masking, flag semantics, and pending-event delivery are Xen backend behavior.
- `uk_pal_except_get_except_stack_base`, `uk_pal_except_push_nested`, `uk_pal_except_pop_nested`: forwarding only.
- `uk_pal_except_init`: returns the backend's `int` result unchanged. Do not invent error codes.

**Do not assume** these wrappers establish correct interrupt masking, exception delivery, or nesting behavior in a Xen guest. Distinguish direct observations from backend contracts that would require separate verification.

## Requirements drafting rules

Write requirements in `Test_Specifications/xen/pal/except.md` using the same layout as the team's `arch_time.md` and `arch_events.md` examples:

1. Title and explicit **draft/example — not approved safety artifact** notice.
2. Source scope and relevant backend dependencies.
3. Concise source observations, including the forwarding classification of each item.
4. Requirements with stable unique IDs, e.g. `REQ-PAL-EXCEPT-001` for the common header and `REQ-PAL-X86-EXCEPT-001` for the x86_64 arch header, each with a triggering condition/precondition, one observable obligation using **shall**, and a trace to an exact repository-relative path and macro or function.
5. Proposed tests with stable unique IDs, e.g. `TEST-PAL-EXCEPT-001` and `TEST-PAL-X86-EXCEPT-001`, links to the requirements they cover, test method, preconditions, ordered steps, and explicit measurable expected results.
6. Assumptions, prerequisites, open questions, and coverage gaps.

Preserve IDs already used in `except.md` when refining existing requirements. Check for duplicate IDs. Do not silently change an ID's meaning or invent a requirement for behavior not established by the headers.

## Testing and evidence

- Select an appropriate method for each claim: source inspection, preprocessor or `_Static_assert` comparison against the Xen backend macros, isolated unit test with stub `uk_plat_xen_*` functions, or Xen-configured build/link verification. Explain what the chosen method actually proves.
- For forwarding tests, consider stubs that record arguments and return distinctive values (for example a recognizable flags value for `save_irqf`, a negative value for `except_init`), **only if a viable harness exists or is explicitly proposed**. Do not invent an existing harness, executable commands, stubs, results, or infrastructure.
- For the `set_irq` / `get_irq` pair, include a value above `0xFFFFFFFF` so the declared `__u32` / `__u64` difference is visible; derive the expected result from the declared types, not from guesses about backend storage.
- Label all unexecuted tests **Proposed — not run**. If tests are eventually run, record the command, configuration, environment, result, and evidence location.
- Ensure each requirement has at least one proposed test and each test identifies its covered requirement(s). A stub-backend test is not proof of Xen guest runtime behavior.

## Review checklist

Before finishing, verify:

- Every macro and inline function in both headers is covered or listed as a coverage gap.
- Source paths and symbol names are exact; any line numbers have been rechecked.
- Requirements are atomic, the forwarded backend is named, and expected results are measurable.
- Requirement and test IDs are unique and cross-referenced.
- Proposed tests are not presented as executed or safety-approved.
- Backend behavior, missing harnesses, and incomplete coverage are stated plainly.

Summarize what `except.md` covers, what remains unverified, and any discrepancies between the document and the current headers. Do not change implementation code as part of a specification-only task.
