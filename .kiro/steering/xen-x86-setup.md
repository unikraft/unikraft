---
inclusion: fileMatch
fileMatchPattern: 'plat/xen/x86/setup.c,Test_Specifications/xen/x86/setup.md'
---

# Xen x86 setup — source-specific steering

## Purpose and scope

Use this guidance when reviewing `plat/xen/x86/setup.c` or drafting/revising `Test_Specifications/xen/x86/setup.md` in this Unikraft fork. Follow `.kiro/steering/xen-test-specifications.md` and `.kiro/steering/xen-x86.md` as the primary project-wide rules. This file adds setup-specific review prompts; it does not replace those rules.

Produce **source-derived, reviewable draft requirements and proposed tests**, not approved safety artifacts. Modify specification documents only unless the user separately requests source changes. Do not claim that any test has been executed without actual execution evidence.

## Inspect source and dependencies first

1. Read the **current checked-out version** of `plat/xen/x86/setup.c` in full. Confirm that the implementation still matches the source used by the existing `setup.md` draft; do not assume earlier code or line numbers remain accurate.
2. Inspect relevant declarations and implementations referenced by the file, including Xen hypercall and startup structures, `ukplat_bootinfo`, memory-region list functions, and helpers called by `_init_mem`. Follow dependencies only far enough to distinguish implemented behavior from assumptions.
3. Inspect configuration guards (including `CONFIG_UKPLAT_MEMRNAME` and `CONFIG_HAVE_X86PKU`) and the relevant build-selection rules. State which behaviors are conditional.
4. Use the **actual code** as evidence. Do not infer guaranteed behavior from function names, comments, or intended platform architecture alone.

## Setup-specific behavior to investigate

Verify each item against the current source before turning it into a requirement:

- `_init_shared_info`: reads the shared-info physical address from `HYPERVISOR_start_info`, requests a virtual-address mapping, crashes on a nonzero hypercall result, and assigns `HYPERVISOR_shared_info` after the mapping call succeeds.
- `_init_mem`: obtains page-frame bounds; applies the `MAX_MEM_SIZE` cap; invokes page-table, bootstrap-clear, and read-only-region helpers; builds a free-memory region; optionally builds an initrd region from module information; inserts regions into boot information; and coalesces the region list. Check error propagation and boundary conditions carefully.
- `_init_segbase`: requests zero-valued FS, user GS, and kernel GS segment bases in sequence and crashes if a request fails.
- `_libxenplat_x86bootinfo_setup`: records the Xen/PVH boot identifiers, initializes memory, and records the command-line pointer and length. Verify dependencies and failure handling.
- `_libxenplat_x86entry`: identify the observable order of segment initialization, boot-info acquisition, trap initialization, start-info assignment, shared-info mapping, event initialization, boot-info setup, early boot initialization, optional PKU check, and boot entry.

**Do not assume** these steps establish complete platform safety, memory isolation, or correct runtime behavior. Distinguish direct observations from external Xen/Unikraft contracts that would require separate verification.

## Requirements drafting rules

Write requirements in `Test_Specifications/xen/x86/setup.md` using the same layout as the team's `arch_time.md` and `arch_events.md` examples:

1. Title and explicit **draft/example — not approved safety artifact** notice.
2. Source scope and relevant dependencies.
3. Concise source observations and control-flow summary.
4. Requirements with stable unique IDs, e.g. `REQ-X86-SETUP-001`, each with a triggering condition/precondition, one observable obligation using **shall**, and a trace to an exact repository-relative path and source function or macro.
5. Proposed tests with stable unique IDs, e.g. `TEST-X86-SETUP-001`, links to the requirements they cover, test method, preconditions, ordered steps, and explicit measurable expected results.
6. Assumptions, prerequisites, open questions, and coverage gaps.

Preserve IDs already used in `setup.md` when refining existing requirements. Check for duplicate IDs. Do not silently change an ID's meaning or invent a requirement for behavior not established by the implementation.

## Testing and evidence

- Select an appropriate method for each claim: source inspection, isolated test with controlled dependencies, build/link verification, or Xen guest integration test. Explain what the chosen method actually proves.
- For initialization ordering, consider an instrumented or mocked call trace, **only if a viable harness exists or is explicitly proposed**. Do not invent an existing harness, executable commands, mocks, results, or infrastructure.
- Include relevant success and failure paths where the source exposes them: failed hypercalls, unavailable boot information, negative memory-region insertion results, optional initrd presence, and configuration-dependent branches.
- For memory-region calculations, examine bounds, page alignment, and the actual helper contracts. Flag uncertain assumptions instead of asserting unsupported guarantees.
- Label all unexecuted tests **Proposed — not run**. If tests are eventually run, record the command, configuration, environment, result, and evidence location.
- Ensure each requirement has at least one proposed test and each test identifies its covered requirement(s). A documentation review is not proof of Xen guest runtime behavior.

## Review checklist

Before finishing, verify:

- Every behavioral claim is supported by the current `setup.c` and, where needed, inspected dependencies.
- Source paths and symbol names are exact; any line numbers have been rechecked.
- Requirements are atomic, conditional behavior is correctly qualified, and expected results are measurable.
- Requirement and test IDs are unique and cross-referenced.
- Proposed tests are not presented as executed or safety-approved.
- Unresolved dependencies, missing harnesses, and incomplete coverage are stated plainly.

Summarize what `setup.md` covers, what remains unverified, and any discrepancies between the document and the current source. Do not change implementation code as part of a specification-only task.
