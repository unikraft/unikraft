#ifndef __UK_CPUID_H__
#define __UK_CPUID_H__

#include <uk/list.h>

#ifdef __cplusplus
extern "C" {
#endif

#define cpuid_macro(ax, bx, cx, dx, func_1, func_2) do { \
		__asm__ __volatile__ ( \
			"cpuid" \
			: "=a" (ax), "=b" (bx), \
			  "=c" (cx), "=d" (dx) \
			: "a" (func_1), "c" (func_2) \
		); \
		} while (0)


#define EAX_VENDOR_INFO 0x0
#define EAX_FEATURE_INFO 0x1
#define EAX_CACHE_INFO 0x2
#define EAX_PROCESSOR 0x3
#define EAX_CACHE_PARAMETERS 0x4
#define EAX_MONITOR_MWAIT_INFO 0x5
#define EAX_THERMAL_POWER_INFO 0x6
#define EAX_STRUCTURED_EXTENDED_FEATURE_INFO 0x7
#define EAX_DIRECT_CACHE_ACCESS_INFO 0x9
#define EAX_PERFORMANCE_MONITOR_INFO 0xA
#define EAX_EXTENDED_TOPOLOGY_INFO 0xB
#define EAX_EXTENDED_STATE_INFO 0xD
#define EAX_RDT_MONITORING 0xF
#define EAX_RDT_ALLOCATION 0x10
#define EAX_SGX 0x12
#define EAX_TRACE_INFO 0x14
#define EAX_TIME_STAMP_COUNTER_INFO 0x15
#define EAX_FREQUENCY_INFO 0x16
#define EAX_SOC_VENDOR_INFO 0x17
#define EAX_DETERMINISTIC_ADDRESS_TRANSLATION_INFO 0x18
#define EAX_EXTENDED_PROCESSOR_INFO_AND_FUTURE_BITS 0x80000001

#define EAX_HIGHEST_BASIC_VALUE 0x0
#define EAX_HIGHEST_EXTENDED_VALUE 0x80000000

struct uk_cpuid_info {
	unsigned int EAX;
	unsigned int EBX;
	unsigned int ECX;
	unsigned int EDX;
};

struct uk_cpuid_cache {
	// TODO include other flags inside this structure

	unsigned char is_xsave_available : 1;
	unsigned char is_xsave_cached : 1;
};

unsigned char is_xsave_available(void);
void init_cpuid(void);
unsigned char get_cpuid_info(unsigned int primary_code,
						unsigned int secondary_code);

#ifdef __cplusplus
}
#endif

#endif
