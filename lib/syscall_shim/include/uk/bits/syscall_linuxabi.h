#ifndef __LIBSYSCALL_SHIM_SYSCALL_LINUXABI_H__
#define __LIBSYSCALL_SHIM_SYSCALL_LINUXABI_H__

#include <uk/lcpu.h>

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
#define __syscall_rip		RCX
#define __syscall_rsyscall	ORIG_RAX
#define __syscall_rarg0		RDI
#define __syscall_rarg1		RSI
#define __syscall_rarg2		RDX
#define __syscall_rarg3		R10
#define __syscall_rarg4		R8
#define __syscall_rarg5		R9

#define __syscall_rret0		RAX
#define __syscall_rret1		RDX

#elif (defined __ARM_64__)
#define __syscall_rip		ELR_EL1
#define __syscall_rsyscall	X8
#define __syscall_rarg0		X0
#define __syscall_rarg1		X1
#define __syscall_rarg2		X2
#define __syscall_rarg3		X3
#define __syscall_rarg4		X4
#define __syscall_rarg5		X5

#define __syscall_rret0		X0
#define __syscall_rret1		X1

#else
#error "Missing register mappings for selected target architecture"
#endif

#endif /* __LIBSYSCALL_SHIM_SYSCALL_LINUXABI_H__ */
