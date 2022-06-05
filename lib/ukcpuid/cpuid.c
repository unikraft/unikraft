#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <uk/errptr.h>
#include <uk/cpuid.h>
#include <uk/init.h>

#include "../plat/common/include/x86/cpu_defs.h"

#define cpuid_macro(ax, bx, cx, dx, func_1, func_2) do { \
		__asm__ __volatile__ ( \
			"cpuid" \
			: "=a" (ax), "=b" (bx), \
			  "=c" (cx), "=d" (dx) \
			: "a" (func_1), "c" (func_2) \
		); \
		} while (0)

#define is_cached(field) (field & CPUID_CACHE_VALID)
#define get_value(field) (field & CPUID_CACHE_VALUE)

#define set_cache(cache_field, value) \
	cache_field |= CPUID_CACHE_VALID; \
	cache_field |= (value & CPUID_CACHE_VALUE);

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

#define CPUID_CACHE_VALID 0x2
#define CPUID_CACHE_VALUE 0x1

struct uk_cpuid_cache {
	// TODO include other flags inside this structure
	__u8 is_AMD : 2;
	__u8 is_INTEL : 2;
	__u8 is_RDRAND_available: 2;

};

static struct uk_cpuid_info info;
static struct uk_cpuid_cache cache;

static __u32 __highest_basic_func_value;
static __u32 __highest_extended_func_value;
static __u32 last_eax_function = 0xaaa;
static __u32 last_ecx_function = 0xaaa;

__u8 is_flag_enabled(__u32 eax_function, __u32 ecx_function,
							  __u32 flag, __u32 *reg)
{

	if (last_eax_function != eax_function ||
		last_ecx_function != ecx_function) {
		last_eax_function = info.EAX = eax_function;
		last_ecx_function = info.ECX = ecx_function;

		cpuid_macro(info.EAX, info.EBX, info.ECX, info.EDX, info.EAX, info.ECX);
	}

	return ((*reg) & flag) == flag;
}

__u8 is_xsave_available(void)
{
	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_ECX_XSAVE, &(info.ECX));
}

__u8 is_osxsave_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_ECX_OSXSAVE, &(info.ECX));
}

__u8 is_avx_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_ECX_AVX, &(info.ECX));
}

__u8 is_fpu_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_EDX_FPU, &(info.EDX));
}


__u8 is_fxsr_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_EDX_FXSR, &(info.EDX));
}

__u8 is_sse_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_EDX_SSE, &(info.EDX));
}

__u8 is_fsgsbase_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_STRUCTURED_EXTENDED_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_STRUCTURED_EXTENDED_FEATURE_INFO, 0x0, X86_CPUID7_EBX_FSGSBASE, &(info.EBX));
}

__u8 is_pku_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_STRUCTURED_EXTENDED_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_STRUCTURED_EXTENDED_FEATURE_INFO, 0x0, X86_CPUID7_ECX_PKU, &(info.ECX));
}

__u8 is_ospke_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_STRUCTURED_EXTENDED_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_STRUCTURED_EXTENDED_FEATURE_INFO, 0x0, X86_CPUID7_ECX_OSPKE, &(info.ECX));
}

__u8 is_xsaveopt_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_EXTENDED_STATE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_EXTENDED_STATE_INFO, 0x1, X86_CPUIDD1_EAX_XSAVEOPT, &(info.EAX));
}


__u8 is_syscall_available(void)
{
	// TODO cache the response ??

	if (__highest_extended_func_value < EAX_EXTENDED_PROCESSOR_INFO_AND_FUTURE_BITS) {
		return 0;
	}

	return is_flag_enabled(EAX_EXTENDED_PROCESSOR_INFO_AND_FUTURE_BITS, 0x0, X86_CPUID3_SYSCALL, &(info.EDX));
}

__u8 is_AMD(void) {
	__u8 value;

	if (is_cached(cache.is_AMD)) {
		goto return_amd;
	}

	if (__highest_basic_func_value < EAX_VENDOR_INFO) {
		return 0;
	}

	info.EAX = EAX_VENDOR_INFO;
	info.ECX = 0x0;
	cpuid_macro(info.EAX, info.EBX, info.ECX, info.EDX, info.EAX, info.ECX);

	value = (memcmp((char *) (&info.EBX), "htuA", 4) == 0
		&& memcmp((char *) (&info.EDX), "itne", 4) == 0
		&& memcmp((char *) (&info.ECX), "DMAc", 4) == 0);

	set_cache(cache.is_AMD, value);

return_amd:
	return get_value(cache.is_AMD);
}

__u8 is_INTEL(void) {
	__u8 value;

	if (is_cached(cache.is_INTEL)) {
		goto return_intel;
	}

	if (__highest_basic_func_value < EAX_VENDOR_INFO) {
		return 0;
	}

	info.EAX = EAX_VENDOR_INFO;
	info.ECX = 0x0;
	cpuid_macro(info.EAX, info.EBX, info.ECX, info.EDX, info.EAX, info.ECX);

	value = (memcmp((char *) (&info.EBX), "Genu", 4) == 0
		&& memcmp((char *) (&info.EDX), "ineI", 4) == 0
		&& memcmp((char *) (&info.ECX), "ntel", 4) == 0);
	
	set_cache(cache.is_INTEL, value);


return_intel:
	return get_value(cache.is_INTEL);
}

__u8 is_RDRAND_available(void) {
	__u8 value;

	if (is_cached(cache.is_RDRAND_available)) {
		goto return_RDRAND;
	}

	if (!(is_INTEL() || is_AMD())) {
		return 0;
	}

	value = is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_ECX_RDRAND, &(info.ECX));
	set_cache(cache.is_RDRAND_available, value);

return_RDRAND:
	return get_value(cache.is_RDRAND_available);
}


// this function will be able to be called from outside for general queries
void get_cpuid_info(__u32 primary_code, __u32 secondary_code, struct uk_cpuid_info* info)
{
	cpuid_macro(info->EAX, info->EBX, info->ECX, info->EDX, primary_code, secondary_code);
}

static int init_cpuid(void)
{	
	info.EAX = EAX_HIGHEST_BASIC_VALUE;
	info.ECX = 0x0;

	cpuid_macro(info.EAX, info.EBX, info.ECX, info.EDX, info.EAX, info.ECX);
	__highest_basic_func_value = info.EAX;

	info.EAX = EAX_HIGHEST_EXTENDED_VALUE;
	cpuid_macro(info.EAX, info.EBX, info.ECX, info.EDX, info.EAX, info.ECX);
	__highest_extended_func_value = info.EAX;

	return 0;
}
uk_rootfs_initcall_prio(init_cpuid, 3);