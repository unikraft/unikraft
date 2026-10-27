---
inclusion: fileMatch
fileMatchPattern: ["Test_Specifications/xen/**/*.md", "plat/xen/**/*"]
---

# Xen capstone test-specification conventions

## Purpose and scope

Help write reviewable, source-derived requirements and proposed tests for this
fork of Unikraft. Store the resulting documents in `Test_Specifications/xen/`,
mirroring the source subdirectories. Start with `x86/`; expand only when asked.
Work on specification documents by default. Source, configuration, and build
changes are separate tasks and must not be introduced just to make a proposed
test pass.

## Evidence before requirements

- Read the target implementation, relevant declarations, direct callers, and
  build-selection rules before describing behavior.
- Use this checkout as the authority for implemented behavior. Clearly label
  external requirements, assumptions, suspected defects, and proposed changes.
- Cite repository-relative source paths and function or macro names for every
  requirement. If including line numbers, verify them against the current file;
  symbols remain the primary anchor.
- Distinguish observed implementation behavior from intended behavior. A
  source-derived requirement documents what the code does; it does not establish
  that the behavior is correct or sufficient for a safety standard.
- Do not infer error handling, concurrency guarantees, supported architectures,
  or timing bounds from function names or comments alone.

## Document structure

Follow `Test_Specifications/xen/x86/arch_events.md` and `arch_time.md`:

1. Title and draft/example status; retain the existing example disclaimer.
2. Source scope, including dependencies needed to interpret behavior.
3. Source observations that explain the relevant control flow.
4. Requirements with stable IDs, a trigger/precondition, one observable
   obligation using "shall", and a source trace.
5. Tests with stable IDs, requirement links, method, preconditions, ordered
   steps, and an explicit expected result.
6. Assumptions, coverage gaps, or open questions when present.

Keep existing IDs stable. Use `REQ-X86-<TOPIC>-NNN` and
`TEST-X86-<TOPIC>-NNN` for new x86 entries; check existing documents for
collisions. Every requirement needs at least one proposed test, and every test
must name the requirements it checks. Do not silently broaden an existing ID's
meaning when adding unrelated behavior.

## Test quality and execution claims

- Select source inspection, an isolated test with controlled dependencies, a
  build/link check, or a Xen guest integration test based on the actual claim.
  State what each method can demonstrate.
- Identify required configuration, initial state, controlled inputs, captured
  operations, expected values, and ordering. Include boundary and failure paths
  when the source contains them; mark absent handling as a gap.
- Separate a proposed harness from one that exists. Do not invent runnable
  commands, fixtures, mocks, or test results.
- Report tests as proposed/not run until executed. For actual runs, record the
  command, configuration, environment, result, and evidence location. A Markdown
  review or successful build is not proof of guest runtime behavior.
- Report missing application configuration, toolchain, Xen host, or harness as
  prerequisites. Do not install dependencies or alter the environment merely to
  complete a documentation task.

## Review before finishing

Recheck source traces, ID uniqueness, requirement/test coverage, exact API and
configuration names, and whether the expected results are measurable. Summarize
the documented scope and remaining gaps without claiming full Xen coverage.
