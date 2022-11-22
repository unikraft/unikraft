#ifndef __UK_CPUID_H__
#define __UK_CPUID_H__

#include <uk/list.h>

#ifdef __cplusplus
extern "C" {
#endif
struct uk_cpuid_info {
	__u32 EAX;
	__u32 EBX;
	__u32 ECX;
	__u32 EDX;
};

__u8 is_xsave_available(void);
void get_cpuid_info(__u32 primary_code, 
					__u32 secondary_code, struct uk_cpuid_info* info);

__u8 is_AMD(void);
__u8 is_INTEL(void);
__u8 is_RDRAND_available(void);
__u8 is_RDSEED_available(void);

#ifdef __cplusplus
}
#endif

#endif
