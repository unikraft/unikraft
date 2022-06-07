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
        movq $0xc0000082, %%rcx; \
        wrmsr; \
        movq $0xc0000080, %%rcx; \
        rdmsr; \
        or $0x1, %%eax; \
        wrmsr; \
        movq $0xc0000081, %%rcx; \
        rdmsr; \
        movl $0x00180008, %%edx; \
        wrmsr; \
        ;\
        movq %0, %%rcx; \
        movq $0x202, %%r11; \
        sysretq;": "=r" (addr));
}