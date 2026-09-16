
#ifndef __UK_SYSCALL_H__
#error Do not include this header directly
#endif

/* RISC-V currently only supports the wrapper/stub path of libsyscall_shim.
 * The Linux binary syscall handler is not wired up, so the execution-environment
 * prologue intentionally expands to nothing.
 */
#if !__ASSEMBLY__

#include <uk/essentials.h>

#define UK_SYSCALL_EXECENV_PROLOGUE_DEFINE(pname, fname, x, ...)

#endif /* !__ASSEMBLY__ */
