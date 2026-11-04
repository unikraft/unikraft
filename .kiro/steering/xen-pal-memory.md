---
inclusion: fileMatch
fileMatchPattern: 'plat/xen/pal/include/uk/plat/pal/addr.h,plat/xen/pal/include/uk/plat/pal/page.h,plat/xen/pal/include/uk/plat/pal/paging.h,plat/xen/pal/include/uk/plat/pal/pt.h,plat/xen/pal/include/uk/plat/pal/tlb.h,Test_Specifications/xen/pal/addr.md,Test_Specifications/xen/pal/page.md,Test_Specifications/xen/pal/paging.md,Test_Specifications/xen/pal/pt.md,Test_Specifications/xen/pal/tlb.md'
---

# Xen PAL memory headers — source-specific steering

## Purpose and scope

Use this guidance when reviewing the PAL memory headers `addr.h`, `page.h`, `paging.h`, `pt.h`, and `tlb.h` in `plat/xen/pal/include/uk/plat/pal/`, or drafting/revising the matching `Test_Specifications/xen/pal/<header>.md` documents in this Unikraft fork. Follow `.kiro/steering/xen-test-specifications.md` and `.kiro/steering/xen-pal.md` as the primary rules. This file adds memory-specific review prompts; it does not replace those rules.

Produce **source-derived, reviewable draft requirements and proposed tests**, not approved safety artifacts. Modify specification documents only unless the user separately requests source changes. Do not claim that any test has been executed without actual execution evidence.

These headers have no arch subdirectory variants, but they expand to architecture-dependent values through the Xen backend. State the architecture (x86_64) that each requirement's expected values assume.

## Inspect source and dependencies first

1. Read the **current checked-out version** of each header in scope in full. Confirm that existing drafts, if any, still match them; do not assume earlier code or line numbers remain accurate.
2. Inspect the Xen backends `plat/xen/include/uk/plat/xen/addr.h`, `page.h`, `paging.h`, `pt.h`, and `tlb.h`, and any arch headers they include. Follow them only far enough to distinguish wrapper facts from backend behavior.
3. Inspect the consumers `lib/ukpaging/include/uk/paging.h` and `lib/ukpaging/arch/x86_64/include/uk/paging/arch.h`, and `lib/ukpaging/Config.uk` (which depends on `HAVE_PAGING`). Determine whether `ukpaging` can be selected together with `CONFIG_PLAT_XEN`.
4. Use the **actual code** as evidence. Do not infer guaranteed behavior from macro names, comments, or intended platform architecture alone.

## Memory-specific behavior to investigate

Verify each item against the current source before turning it into a requirement:

- `paging.h`: contains only its include guard and a comment that the Xen platform does not currently support paging.
- `addr.h`: defines `UK_PAL_VADDR_INV` and `UK_PAL_PADDR_INV` (mapped to `UK_PLAT_XEN_*`) and states in a comment that address functions are not declared.
- `pt.h`: defines `UK_PAL_PT_LEVELS`, `UK_PAL_PTES_PER_LEVEL`, `UK_PAL_PT_LEVEL_SHIFT`, and the `UK_PAL_PT_Lx_*` macros, and states in a comment that page-table operations are not defined. Note which items are assembly-visible.
- `page.h`: base, large, and huge page level/shift/size/mask macros; the `Lx_SHIFT`, `SHIFT_Lx`, `Lx_SIZE`, `Lx_MASK`, and align-up/align-down/aligned families; `UK_PAL_PAGE_ATTR_PROT_*`; and `UK_PAL_PAGE_Lx_HAS` / `Lx_IS` inside `#if !__ASSEMBLY__`. Confirm each maps to the same-named `UK_PLAT_XEN_PAGE_*` macro with parameters passed in the same order.
- `tlb.h`: `uk_pal_tlb_flush_entry(vaddr)` and `uk_pal_tlb_flush()` forward to `uk_plat_xen_tlb_flush_entry` and `uk_plat_xen_tlb_flush`.

**Do not assume** these headers establish correct paging, page-table, or TLB behavior in a Xen guest. What a flush does in Xen (for example through a hypercall) and the correctness of the backend alignment arithmetic are backend behavior requiring separate verification.

## Requirements drafting rules

Write one specification per header (`addr.md`, `page.md`, `paging.md`, `pt.md`, `tlb.md`) using the same layout as the team's `arch_time.md` and `arch_events.md` examples:

1. Title and explicit **draft/example — not approved safety artifact** notice.
2. Source scope and relevant backend dependencies.
3. Concise source observations, including what each header defines and what it deliberately omits.
4. Requirements with stable unique IDs, e.g. `REQ-PAL-PAGE-001` or `REQ-PAL-TLB-001`, each with a triggering condition/precondition, one observable obligation using **shall**, and a trace to an exact repository-relative path and macro or function.
5. Proposed tests with stable unique IDs, e.g. `TEST-PAL-PAGE-001`, links to the requirements they cover, test method, preconditions, ordered steps, and explicit measurable expected results.
6. Assumptions, prerequisites, open questions, and coverage gaps.

Write requirements for what is defined and, separately, for what is deliberately absent, but only where the header itself shows the absence. If a consumer could need a missing item and the configuration does not prevent it, record an open question; do not claim a defect without a failing build. Preserve existing IDs and check for duplicates.

## Testing and evidence

- Select an appropriate method for each claim: source inspection, `_Static_assert` comparison of `UK_PAL_*` constants with Xen backend constants, compile-time checks of function-like macros at boundary inputs, isolated unit test with stub backend functions, or Xen-configured build verification. Explain what the chosen method actually proves.
- For align macros, compare PAL and backend results at boundary inputs such as `0`, an already-aligned value, `PAGE_SIZE - 1`, and `PAGE_SIZE + 1`. This proves the PAL and backend agree, not that the backend arithmetic is correct or overflow-safe; record behavior near the top of the address range as a backend question.
- For absence requirements, propose a translation unit that references the undeclared item and is expected to fail to compile in a Xen build.
- For `tlb.h`, consider a stub recording the `vaddr` passed and the call count, **only if a viable harness exists or is explicitly proposed**. Do not invent an existing harness, executable commands, stubs, results, or infrastructure.
- Label all unexecuted tests **Proposed — not run**. If tests are eventually run, record the command, configuration, environment, result, and evidence location.
- Ensure each requirement has at least one proposed test and each test identifies its covered requirement(s).

## Review checklist

Before finishing, verify:

- Every macro and inline function in the headers in scope is covered or listed as a coverage gap, including deliberate absences.
- Source paths and symbol names are exact; any line numbers have been rechecked.
- Requirements are atomic, the assumed architecture is stated, and expected results are measurable.
- Requirement and test IDs are unique and cross-referenced.
- Proposed tests are not presented as executed or safety-approved.
- Backend behavior, the paging-support question, missing harnesses, and incomplete coverage are stated plainly.

Summarize what each document covers, what remains unverified, and any discrepancies between the documents and the current headers. State plainly that runtime paging, page-table, and TLB behavior in a Xen guest is not established by these documents. Do not change implementation code as part of a specification-only task.
