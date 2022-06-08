#include <uk/sgx_internal.h>
#include <uk/sgx_asm.h>
#include <uk/config.h>
/*
 * Actually unikernel does not need ring-3, however,
 * ENCLU instruction in only avaliable in ring-3. Hence,
 * we need somehow to switch the CPL from ring 0 to ring 3
 * when the CPU is running enclave code.
 */
void switch_to_ring3()
{
#ifdef CONFIG_KVM_RING3
	// Set up a stack structure for switching to user mode.
	asm volatile("  \
      mov $0x23, %rax; \
      mov %rax, %ds; \
      mov %rax, %es; \
      mov %rsp, %rax; \
      push $0x23; \
      push %rax; \
      pushf; \
      push $0x1B; \
      push $1f; \
      iretq; \
    1: \
      "); 
// (void *)0;
// asm volatile(" \
//         nop; \
//         nop; \
//         nop; \
//         nop; \
//         nop; \
//         nop; \
//         nop; \
//         nop; \
//         nop; \
//         nop; ");
#endif
        return;
}