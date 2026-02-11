# ukpcpuvar — Per-CPU Variables

`libukpcpuvar` provides efficient per-CPU variable support for multicore systems.
Per-CPU variables allow each CPU to maintain its own copy of a piece of data, eliminating the need for locks when accessing CPU-local state and preventing false sharing through cache-line-aligned storage.

## Overview

Variables are declared with the `__uk_pcpuvar` attribute, which places them into a dedicated `.uk_pcpuvar` ELF section.
At link time the linker script treats this section as a **template** and reserves contiguous space for `CONFIG_UKPLAT_CPU_MAXCOUNT` copies of it, each padded to a cache line boundary.
A post-build script (`mkukpcpuvar.py`) then materialises the remaining CPU copies by replicating the template data directly into the binary, so all CPUs start with identical initial values without runtime allocation or manual duplication.

Because per-CPU variables live inside the image itself, they are **always accessible** — there is no separate allocation step that could fail or not yet have run.
This makes them safe to reference even from the earliest stages of the boot process, before a heap or memory allocator exists.

## Design

### Section Layout

```
.uk_pcpuvar
├── [CPU 0 copy]   ← template (defined by source code)
├── [CPU 1 copy]   ← replicated by mkukpcpuvar.py
├── [CPU 2 copy]
│    ...
└── [CPU N-1 copy]
```

Each copy is aligned to `UK_ARCH_CACHE_LINE_SIZE` to prevent false sharing.
The total section size is reserved at link time; `mkukpcpuvar.py` fills in the copies after linking but before stripping.

### Addressing

At runtime, each CPU's copy is reached by offsetting from the template symbol address by `idx * _uk_pcpuvar_tmpl_size`:

- **x86_64**: The GS base register is pre-loaded with the CPU's slot offset, so any per-CPU access compiles to a single GS-relative RIP instruction — `%gs:sym(%rip)` — with no extra arithmetic at the access site.
- **arm64**: `TPIDR_EL1` holds the equivalent slot offset; accesses compute `TPIDR_EL1 + (sym - _uk_pcpuvar_base)` at the access site.

### Referencing a Symbol Directly

Referencing a per-CPU variable by its symbol name (e.g. `&uk_pcpuvar_cpu_id`) **always gives the address of the bootstrap CPU's copy** (CPU 0, the template).
This is intentional and useful: during early boot, before per-CPU registers have been configured, you can still safely read and write the BSP's own per-CPU data directly via the symbol, since the BSP *is* CPU 0 at that point.
Code that needs to access another CPU's copy explicitly should use `uk_pcpuvar_lval(idx, sym)` instead.

## Post-Build Script: `mkukpcpuvar.py`

The script runs after linking and performs the following:

1. **Locates** the `.uk_pcpuvar` section by scanning the ELF section headers.
2. **Reads** the template boundaries and cache line alignment from the linker-exported symbols `_uk_pcpuvar_tmpl_start`, `_uk_pcpuvar_tmpl_end`, `_uk_pcpuvar_tmpl_size`, and `_uk_pcpuvar_align` via `nm`.
3. **Validates** that the section has sufficient reserved space for all CPU copies, and that no adjacent section (located by file offset, not by section header index order) would be overwritten.
4. **Replicates** the CPU 0 template `max_cpus` times, padding each copy to the cache-line-aligned size, and writes the result back into the binary in-place.

It is invoked via the `build_uk_pcpuvar` make macro defined in `Makefile.rules`, called from the platform linker rules after the main link step.

## API

### Declaring a Per-CPU Variable

```c
#include <uk/pcpuvar.h>

__uk_pcpuvar int my_counter;
__uk_pcpuvar struct my_state cpu_state;
```

### Accessing Variables

| Macro | Description |
|---|---|
| `uk_pcpuvar_lval(idx, sym)` | Lvalue for CPU `idx`'s copy (read or write) |
| `uk_pcpuvar_current_get(sym)` | Read current CPU's copy |
| `uk_pcpuvar_current_set(sym, val)` | Write current CPU's copy |
| `uk_pcpuvar_current_ptr_get(sym)` | Pointer to current CPU's copy |
| `uk_pcpuvar_current_member_get(sym, member)` | Read struct member from current CPU's copy |
| `uk_pcpuvar_current_member_set(sym, member, val)` | Write struct member to current CPU's copy |

### Built-In Per-CPU Variables

```c
__uk_pcpuvar __u64 uk_pcpuvar_cpu_id;   /* hardware CPU/APIC ID/MPIDR_EL1 */
__uk_pcpuvar __u64 uk_pcpuvar_cpu_idx;  /* linear array index   */
```

## Early Boot Considerations

### Per-CPU Register Not Yet Initialized

`uk_pcpuvar_current_get` / `uk_pcpuvar_current_set` and their member variants rely on the per-CPU register (GS base on x86_64, `TPIDR_EL1` on arm64) being correctly set up for the running CPU.
**Before this initialization has taken place, calling these accessors produces undefined behavior.**
If you need to access a per-CPU variable in very early boot code before the per-CPU register is ready, use `uk_pcpuvar_lval(0, sym)` to explicitly address the BSP's copy, or reference the symbol directly if you know you are running on the BSP.

### Uninitialized Copies

During early boot it is not safe to read a per-CPU variable your current component does not own unless you know the component that does own it has already initialized them.
The post-build script replicates the BSP template's compile-time initial values to all CPU slots, but any variable that is initialized at runtime (e.g. `uk_pcpuvar_cpu_id`) will only hold a meaningful value in a given CPU's slot once that CPU or some designated initializer has written it.
Accessing a slot before its owner has initialized it will silently return stale data.

## Architecture-Specific Implementation Notes

Each architecture provides a header at `arch/<arch>/include/uk/pcpuvar/arch.h` that implements:

- `__uk_pcpuvar_arch_current_get(sym)`
- `__uk_pcpuvar_arch_current_set(sym, val)`
- `__uk_pcpuvar_arch_current_member_get(sym, member)`
- `__uk_pcpuvar_arch_current_member_set(sym, member, val)`
- `__uk_pcpuvar_arch_current_ptr_get(sym)`

And an assembly macro for computing a CPU slot's register value from an index (each architecture can define its own variant if needed, this is **not mandated**):

- x86_64: `uk_pcpuvar_x86_64_gsval dest, idx`
- arm64: `uk_pcpuvar_arm64_tpidrval dest, idx`

### Note on `_uk_pcpuvar_tmpl_size` and `ABSOLUTE()`

The linker script defines the template size as:

```
_uk_pcpuvar_tmpl_size = ABSOLUTE(_uk_pcpuvar_tmpl_end - _uk_pcpuvar_tmpl_start);
```

This is worth noting for anyone adding a new architecture.
Without `ABSOLUTE()`, a difference between two symbols in the same section is section-relative, and the toolchain will emit a load-time relocation for any reference to it.
That relocation must be resolved before the value can be used — which may not yet have happened in early boot assembly.

`ABSOLUTE()` forces the linker to evaluate the expression as a pure integer constant at link time.
The benefit is concrete and architecture-specific:

- **x86_64**: The `uk_pcpuvar_x86_64_gsval` assembly macro expands to:
  ```asm
  movabsq $_uk_pcpuvar_tmpl_size, %reg
  imulq   %idx, %reg
  ```
  Because `_uk_pcpuvar_tmpl_size` is an absolute symbol, `movabsq` encodes it as an immediate with no relocation entry.
  This makes it safe to use in the earliest boot stages, before any relocation fixup code has run.

- **arm64**: Even when the value is accessed via a literal pool (`ldr reg, =_uk_pcpuvar_tmpl_size`) which are generally unsafe in pre-relocation code, `ABSOLUTE()` still matters: an absolute symbol causes the assembler to resolve its entry in the literal pool to a plain integer at assemble/link time rather than emitting an `R_AARCH64_ABS64` relocation that needs runtime relocation.
  The net effect is the same zero-relocation property as on x86_64, again safe for early boot use.

For a new architecture, as long as the per-CPU slot offset can be expressed as `idx * _uk_pcpuvar_tmpl_size` and that size is an absolute link-time constant, this same property will hold.

## What This Library Does Not Provide

`libukpcpuvar` intentionally limits itself to three responsibilities:

1. The `.uk_pcpuvar` linker section and its duplication at post-build time.
2. The compile-time and inline-assembly accessors.
3. The built-in `uk_pcpuvar_cpu_id` and `uk_pcpuvar_cpu_idx` variables.

It does **not** provide:

- Any API for initializing the per-CPU register (GS base, `TPIDR_EL1`, etc.).
- Any constructor or destructor mechanism for per-CPU variables.
- Any CPU discovery or bring-up logic.

This is a deliberate design choice.
The library cannot generically define how the per-CPU register is set up, because:

- Different **platforms** initialize it differently (e.g., on a standard KVM/x86_64 platform this is a plain `wrgsbase` or MSR write, but on Xen PV a hypercall would be a better fit).
  A generic initialization API would need to be applicable to all such cases.
- Different **architectures** may not even have a dedicated per-CPU register (e.g. RISC-V likely has no architectural register with clear ownership that maps to this role — the port author will need to decide what the architecture-specific accessor header looks like and which register or mechanism to use).
- The library cannot know **how early** a platform needs per-CPU variable access to be available.
  Some platforms may need it before C code can even run, at a point where no library initialization callback can execute.

It is therefore the responsibility of a **pcpuvar-aware component** — typically platform boot code or a designated initializer — to initialize the per-CPU register (or reference component) for each CPU at the appropriate point during startup, and to populate variables like `uk_pcpuvar_cpu_id` and `uk_pcpuvar_cpu_idx` before any code that relies on them runs.
