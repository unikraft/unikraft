---
inclusion: fileMatch
fileMatchPattern: 'plat/xen/pal/include/uk/plat/pal/regs.h,plat/xen/pal/include/uk/plat/pal/sysctx.h,plat/xen/pal/include/uk/plat/pal/ectx.h,plat/xen/pal/include/uk/plat/pal/auxsp.h,plat/xen/pal/include/uk/plat/pal/start.h,plat/xen/pal/arch/x86_64/include/uk/plat/pal/arch/regs.h,plat/xen/pal/arch/x86_64/include/uk/plat/pal/arch/sysctx.h,Test_Specifications/xen/pal/regs.md,Test_Specifications/xen/pal/sysctx.md,Test_Specifications/xen/pal/ectx.md,Test_Specifications/xen/pal/auxsp.md,Test_Specifications/xen/pal/start.md'
---

# Xen PAL CPU context headers — source-specific steering

## Purpose and scope

Use this guidance when reviewing the PAL headers for CPU state and startup symbols, `regs.h`, `sysctx.h`, `ectx.h`, `auxsp.h`, and `start.h` in `plat/xen/pal/include/uk/plat/pal/`, plus the x86_64 `arch/regs.h` and `arch/sysctx.h`, or drafting/revising the matching `Test_Specifications/xen/pal/<header>.md` documents in this Unikraft fork. Follow `.kiro/steering/xen-test-specifications.md` and `.kiro/steering/xen-pal.md` as the primary rules. This file adds context-specific review prompts; it does not replace those rules.

Produce **source-derived, reviewable draft requirements and proposed tests**, not approved safety artifacts. Modify specification documents only unless the user separately requests source changes. Do not claim that any test has been executed without actual execution evidence.

## Inspect source and dependencies first

1. Read the **current checked-out version** of each header in scope in full. Confirm that existing drafts, if any, still match them; do not assume earlier code or line numbers remain accurate.
2. Inspect the backends: `plat/native/include/uk/plat/native/regs.h`, `sysctx.h`, and `ectx.h`, and `plat/xen/include/uk/plat/xen/sysctx.h`, `auxsp.h`, and `start.h`. Follow them only far enough to distinguish wrapper facts from backend behavior.
3. Inspect the contract headers in `lib/ukpal/include/uk/pal/` (`regs.h`, `sysctx.h`, `ectx.h`, `start.h`) and the consumers in `lib/uklcpu/include/uk/lcpu/`. Note the structure-size `#error` check in `lib/uklcpu/include/uk/lcpu/regs.h` and whether a PAL value feeds it.
4. Use the **actual code** as evidence. Do not infer guaranteed behavior from function names, comments, or intended platform architecture alone; the headers' own comments state intent, not proof.

## Context-specific behavior to investigate

Verify each item against the current source before turning it into a requirement:

- `regs.h`: native-forwarded. `UK_PAL_REGS_OFFSETOF_SP`, `UK_PAL_REGS_OFFSETOF_PC`, and `UK_PAL_REGS_SIZE` map to native macros; `uk_pal_regs_get` and `uk_pal_regs_set` forward to `uk_plat_native_regs_get/set` with the byte `offset` passed through unchecked.
- x86_64 `arch/regs.h`: 21 `UK_PAL_X86_64_REGS_OFFSETOF_*` macros (R15 through SS) map to their native counterparts. Count them against the current file.
- `ectx.h`: native-forwarded. Load/store/sanitize function symbols, `UK_PAL_ECTX_SIZE`, `UK_PAL_ECTX_ALIGN`, and the wrappers `uk_pal_ectx_sanitize`, `_store`, `_load`, `_init`, and `_assert_equal`.
- `sysctx.h`: **mixed**. Native layout (`UK_PAL_SYSCTX_SIZE`, `UK_PAL_SYSCTX_OFFSETOF_TLSP`, `uk_pal_sysctx_get/set`) with Xen operations (`UK_PAL_SYSCTX_LOAD_FNSYM`, `UK_PAL_SYSCTX_STORE_FNSYM`, `uk_pal_sysctx_store/load`, `uk_pal_tlsp_get/set`). The Xen store/load functions take `struct uk_plat_native_sysctx *`; `store` and `load` are `__isr`, the others are not.
- x86_64 `arch/sysctx.h`: `UK_PAL_X86_64_SYSCTX_OFFSETOF_GSBASE` and `_FSBASE` map to native macros.
- `auxsp.h`: `UK_PAL_AUXSP_SYM` and `uk_pal_set_auxsp` forward to Xen.
- `start.h`: `UK_PAL_SENTRY_SYM`, `UK_PAL_SSTACKP_SYM`, and `UK_PAL_SARG_SYM` map to Xen symbol macros; the header defines no functions.

**Do not assume** these wrappers establish correct context save/restore, TLS pointer setup, or startup behavior. What the Xen sysctx and TLS functions do (for example any hypercall to set a segment base) is backend behavior requiring separate verification.

## Requirements drafting rules

Write one specification per common header (`regs.md`, `sysctx.md`, `ectx.md`, `auxsp.md`, `start.md`), covering the x86_64 arch header in its own section where one exists. Use the same layout as the team's `arch_time.md` and `arch_events.md` examples:

1. Title and explicit **draft/example — not approved safety artifact** notice.
2. Source scope and relevant backend dependencies.
3. Concise source observations, including which backend each item uses.
4. Requirements with stable unique IDs, e.g. `REQ-PAL-SYSCTX-001` and `REQ-PAL-X86-REGS-001`, each with a triggering condition/precondition, one observable obligation using **shall**, and a trace to an exact repository-relative path and macro or function. For `sysctx.h`, write separate requirements for the native-forwarded and Xen-forwarded halves.
5. Proposed tests with stable unique IDs, e.g. `TEST-PAL-SYSCTX-001` and `TEST-PAL-X86-REGS-001`, links to the requirements they cover, test method, preconditions, ordered steps, and explicit measurable expected results.
6. Assumptions, prerequisites, open questions, and coverage gaps.

Preserve existing IDs when refining existing requirements. Check for duplicate IDs. A block of offset macros may share one requirement with a mapping table. Do not claim bounds checking on `offset` arguments or invent requirements for behavior not established by the headers.

## Testing and evidence

- Select an appropriate method for each claim: source inspection, `_Static_assert` comparison of each `UK_PAL_*` value with its backend value, `offsetof` checks against the native structures (labeled as a check of the native contract), isolated unit test with stub backend functions, or Xen-configured build/link verification for startup symbols. Explain what the chosen method actually proves.
- For `sysctx.h`, propose one test for the native-forwarded half and one for the Xen-forwarded half, so a wrong backend shows up as a distinct failure.
- Use stubs recording pointer, offset, and value arguments **only if a viable harness exists or is explicitly proposed**. Do not invent an existing harness, executable commands, stubs, results, or infrastructure.
- Label all unexecuted tests **Proposed — not run**. If tests are eventually run, record the command, configuration, environment, result, and evidence location.
- Ensure each requirement has at least one proposed test and each test identifies its covered requirement(s). A build or stub test is not proof of Xen guest runtime behavior.

## Review checklist

Before finishing, verify:

- Every macro and inline function in the headers in scope is covered or listed as a coverage gap.
- Each requirement names the backend (Xen or native) as observed in the current header.
- Source paths and symbol names are exact; any line numbers have been rechecked.
- Requirement and test IDs are unique and cross-referenced.
- Proposed tests are not presented as executed or safety-approved.
- Backend behavior, layout-compatibility assumptions, missing harnesses, and incomplete coverage are stated plainly.

Summarize what each document covers, what remains unverified, and any discrepancies between the documents and the current headers. Do not change implementation code as part of a specification-only task.
