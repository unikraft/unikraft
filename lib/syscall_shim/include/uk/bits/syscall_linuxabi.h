#ifndef __LIBSYSCALL_SHIM_SYSCALL_LINUXABI_H__
#define __LIBSYSCALL_SHIM_SYSCALL_LINUXABI_H__

#include <uk/arch/lcpu.h>

/*
 * Mappings of `struct __reg` register fields
 * according to Linux ABI definition for system calls
 * (see: man syscall(2))
 *  rip      - Instruction pointer
 *  rsyscall - Syscall number
 *  rargX    - Arguments 0..5
 *  rretX    - System call return values 0..1
 */

#if (defined __X86_64__)
#define __syscall_rip		rcx
#define __syscall_rsyscall	orig_rax
#define __syscall_rarg0		rdi
#define __syscall_rarg1		rsi
#define __syscall_rarg2		rdx
#define __syscall_rarg3		r10
#define __syscall_rarg4		r8
#define __syscall_rarg5		r9

#define __syscall_rret0		rax
#define __syscall_rret1		rdx

#elif (defined __ARM_64__)
#define __syscall_rip		elr_el1
#define __syscall_rsyscall	x[8]
#define __syscall_rarg0		x[0]
#define __syscall_rarg1		x[1]
#define __syscall_rarg2		x[2]
#define __syscall_rarg3		x[3]
#define __syscall_rarg4		x[4]
#define __syscall_rarg5		x[5]

#define __syscall_rret0		x[0]
#define __syscall_rret1		x[1]

#else
#error "Missing register mappings for selected target architecture"
#endif

#endif /* __LIBSYSCALL_SHIM_SYSCALL_LINUXABI_H__ */
