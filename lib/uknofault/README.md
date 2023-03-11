# uknofault

This library provides support for performing fault-safe memory accesses and
`memcpy`s.

Normally, when the CPU attempts to access a memory address that is not
accessible or if the access would violate the protection settings of the
corresponding page, a page fault occurs. Similarly, on x86 the CPU raises a
general protection fault when a non-canonicalized virtual address is used.
Writing code that can potentially be given invalid addresses or which might
attempt accesses to inaccessible memory thus becomes very complicated.

This library provides functions for performing fault-safe memory accesses.
Faults will just cause the functions to indicate an error instead of crashing.

Generally, all functions are compiled as ISR-safe. `uknofault` also provides
variants that will in addition disable on-demand paging for duration of the
access or continue on failure (e.g., to make sure a whole region is not
accessible). These variants can be selected via the flags parameter.

## Example 1)
The following code checks if a given address range [0x100000; 0x100000+42) can
be read:
```C
#include <uk/nofault.h>

if (uk_nofault_probe_r(0x100000, 42, 0) != 42)
        uk_pr_err("Cannot read memory range\n");
```

## Example 2)
The following code performs a fault-safe `memcpy`. Both the `src` and `dst` are
allowed to cause faults:
```C
#include <uk/nofault.h>

char *dst = ...;
const char *src = ...;
__sz l, len = ...;

l = uk_nofault_memcpy(dst, src, len, 0);
if (l != len)
        uk_pr_err("Could only copy %ld bytes\n", l);
```

## Example 3)
The following code disables on-demand paging while performing the probe:
```C
#include <uk/nofault.h>

if (uk_nofault_probe_r(0x100000, 42, UK_NOFAULTF_NOPAGING) != 42)
        uk_pr_err("Cannot read memory range\n");
```
