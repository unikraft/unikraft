#include <uk/sgx_internal.h>
#include <uk/sgx_asm.h>

/* 
 * Actually unikernel does not need "user mode", however,
 * ENCLU instruction in only avaliable in user mode. Hence,
 * we need somehow to switch the CPL from ring 0 to ring 3
 * when the CPU is running enclave code.
 */
void switch_to_user_mode()
{
	// Set up a stack structure for switching to user mode.
	asm volatile("  \
     cli; \
     mov $0x23, %ax; \
     mov %ax, %ds; \
     mov %ax, %es; \
     mov %ax, %fs; \
     mov %ax, %gs; \
                   \
     mov %esp, %eax; \
     pushl $0x23; \
     pushl %eax; \
     pushf; \
     pushl $0x1B; \
     push $1f; \
     iret; \
   1: \
     ");
}