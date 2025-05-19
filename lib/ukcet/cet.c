#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <uk/vmem.h>
#include <uk/cet.h>

#define PAGE_ATTR_PROT_READ		0x01 /* Page is readable */
#define X86_PTE_DIRTY			0x040UL
//#define PAGE_SIZE 4096

static inline void _cpuid(__u32 fn, __u32 subfn,
				    __u32 *eax, __u32 *ebx,
				    __u32 *ecx, __u32 *edx)
{
	__asm__ __volatile__("cpuid"
			     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
			     : "a"(fn), "c" (subfn));
}

int ukcet_cpu_supports_shadow_stack() {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    _cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    return (ecx & (1 << 7)) != 0;
}

int ukcet_cpu_supports_ibt() {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    _cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    return (edx & (1 << 20)) != 0;
}

void* ukcet_create_shstk() {
	void *mem = mmap(NULL, SHSTK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mem == MAP_FAILED) {
		return NULL;
	}

	for (unsigned int i = 0; i < SHSTK_SIZE; i += PAGE_SIZE) {
		((char*)mem)[i] = 0;
	}

	// create a supervisor stack token
	void *stack_start = ((char*)mem) + SHSTK_SIZE - PAGE_SIZE - 8;
	*(unsigned long long*)stack_start = ((unsigned long long) stack_start);

	// set the shadow stack page attributes
	struct uk_vas* vas = uk_vas_get_active();
	if (vas == NULL) {
		return NULL;
	}
	int rc = uk_vma_set_attr(vas, (unsigned long) mem, SHSTK_SIZE, PAGE_ATTR_PROT_READ | X86_PTE_DIRTY, 0);
	if (rc != 0) {
		return NULL;
	}
	return mem;
}

void* ukcet_create_isst() {
	unsigned long long* isst = mmap(NULL, 8 * sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (isst == NULL) {
		//printf("failed to init ISST\n");
		return NULL;
	}
	for (int i = 1; i < 8; i++) {
		void* shstk = ukcet_create_shstk(SHSTK_SIZE);
		if (shstk == NULL) {
			//printf("failed to init ISST shadow stack at index %d\n", i);
			return NULL;
		}
		isst[i] = (unsigned long long)(((char*)shstk) + SHSTK_SIZE - PAGE_SIZE - 8);
		
		//printf("ISST entry created at %d: %llx token:%llx\n", i, isst[i], ((unsigned long long*)isst[i])[0]);
	}
	//wrmsrl(MSR_IA32_INT_SSP_TAB, (unsigned long long) isst);
	return (void*)isst;
}

void ukcet_unmap_isst(void *isst) {
    for (int i = 1; i < 8; i++) {
		munmap((void*)(((unsigned long long*)(isst))[i]), SHSTK_SIZE);
	}
	munmap(isst, 8 * sizeof(unsigned long long));
}
