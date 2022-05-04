#ifndef _UK_SGX_CPU_H_
#define _UK_SGX_CPU_H_

#include <uk/arch/types.h>

#define	BIT(nr)			        (1UL << (nr))

/* Referred to as IA32_FEATURE_CONTROL in Intel's SDM. */
#define X86_MSR_IA32_FEAT_CTL		            0x0000003a
#define X86_FEAT_CTL_LOCKED				        BIT(0)
#define X86_FEAT_CTL_VMX_ENABLED_INSIDE_SMX		BIT(1)
#define X86_FEAT_CTL_VMX_ENABLED_OUTSIDE_SMX	BIT(2)
#define X86_FEAT_CTL_SGX_LC_ENABLED			    BIT(17)
#define X86_FEAT_CTL_SGX_ENABLED			    BIT(18)
#define X86_FEAT_CTL_LMCE_ENABLED			    BIT(20)

static inline void rdmsr(unsigned int msr, __u32 *lo, __u32 *hi)
{
	asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi)
			     : "c"(msr));
}

static inline __u64 rdmsrl(unsigned int msr)
{
	__u32 lo, hi;

	rdmsr(msr, &lo, &hi);
	return ((__u64) lo | (__u64) hi << 32);
}

#endif