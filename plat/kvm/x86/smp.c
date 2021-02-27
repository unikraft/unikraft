#include <uk/plat/lcpu.h>
#include <uk/arch/types.h>

#define CPUID_FEAT_EDX_APIC (1 << 9)

static inline char check_apic()
{
	unsigned int eax, ebx, ecx, edx;

	eax = 1;
	asm volatile("cpuid"
		     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		     : "0"(*eax), "2"(*ecx));

	/* return 1 if the 9th bit of EDX is set,
	 * meaning that the CPU supports apic
	 */
	return (edx & CPUID_FEAT_EDX_APIC);
}

__u8 ukplat_lcpu_count()
{
	__u8 numcore;

	__u8 *ptr, *ptr2;
	__u32 len;

	__u8 *rsdt;

	// iterate on ACPI table pointers
	for (len = *((__u32 *)(rsdt + 4)), ptr2 = rsdt + 36; ptr2 < rsdt + len;
	     ptr2 += rsdt[0] == 'X' ? 8 : 4) {
		ptr = (__u8 *)(__uptr)(rsdt[0] == 'X' ? *((__u64 *)ptr2)
						      : *((__u32 *)ptr2));
		if (!memcmp(ptr, "APIC", 4)) {
			// found MADT
			lapic_ptr = (__u64)(*((__u32)(ptr + 0x24)));
			ptr2 = ptr + *((__u32 *)(ptr + 4));

			// iterate on variable length records
			for (ptr += 44; ptr < ptr2; ptr += ptr[1]) {
				if (ptr[0] == 0) {
					if (ptr[4] & 1) // found Processor Local APIC
						numcore++;
				}
			}
			break;
		}
	}

	return numcore;
}