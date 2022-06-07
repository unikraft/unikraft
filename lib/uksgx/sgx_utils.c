#include <uk/sgx_internal.h>
#include <uk/sgx_asm.h>

/*
 * Actually unikernel does not need ring-3, however,
 * ENCLU instruction in only avaliable in ring-3. Hence,
 * we need somehow to switch the CPL from ring 0 to ring 3
 * when the CPU is running enclave code.
 */
void switch_to_ring3(__u64 addr)
{
	// Set up a stack structure for switching to user mode.
	asm volatile("  \
        cli; \
        mov $0x23, %rax; \
        mov %rax, %ds; \
        mov %rax, %es; \
        mov %rax, %fs; \
        mov %rax, %gs; \
                        \
        mov %rsp, %rax; \
        push $0x23; \
        push %rax; \
        pushf; \
        push $0x1B; \
        push $1f; \
        iret; \
        1: \
     ");
}