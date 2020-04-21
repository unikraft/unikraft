#ifndef _REGMAP_LINUXABI_H_

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
#define rip		rcx
#define rsyscall	orig_rax
#define rarg0		rdi
#define rarg1		rsi
#define rarg2		rdx
#define rarg3		r10
#define rarg4		r8
#define rarg5		r9

#define rret0		rax
#define rret1		rdx

#elif (defined __ARM64__)
#define rip		x[15] /* TODO: Is this correct? */
#define rsyscall	x[8]
#define rarg0		x[0]
#define rarg1		x[1]
#define rarg2		x[2]
#define rarg3		x[3]
#define rarg4		x[4]
#define rarg5		x[5]

#define rret0		x[0]
#define rret1		x[1]

#else
#error "Missing register mappings for selected target architecture"

#endif

#endif /* _REGMAP_LINUXABI_H_ */
