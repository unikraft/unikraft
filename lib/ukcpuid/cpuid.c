#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <uk/errptr.h>
#include <uk/cpuid.h>

#include "../plat/common/include/x86/cpu_defs.h"

static struct uk_cpuid_info info;
static struct uk_cpuid_cache cache;

static unsigned int __highest_basic_func_value;
static unsigned int __highest_extended_func_value;
static unsigned int last_eax_function = -1;
static unsigned int last_ecx_function = -1;


unsigned char is_flag_enabled(unsigned int eax_function, unsigned int ecx_function,
							  unsigned int flag, unsigned int *reg)
{

	if (last_eax_function != eax_function ||
		last_ecx_function != ecx_function) {
		last_eax_function = info.EAX = eax_function;
		last_ecx_function = info.ECX = ecx_function;

		cpuid_macro(info.EAX, info.EBX, info.ECX, info.EDX, info.EAX, info.ECX);
	}

	return (*reg) & flag;
}

void init_cpuid(void)
{
	info.EAX = EAX_HIGHEST_BASIC_VALUE;
	info.ECX = 0x0;

	cpuid_macro(info.EAX, info.EBX, info.ECX, info.EDX, info.EAX, info.ECX);
	__highest_basic_func_value = info.EAX;

	info.EAX = EAX_HIGHEST_EXTENDED_VALUE;
	cpuid_macro(info.EAX, info.EBX, info.ECX, info.EDX, info.EAX, info.ECX);
	__highest_extended_func_value = info.EAX;
}

unsigned char is_xsave_available(void)
{
	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	if (cache.is_xsave_cached) {
		goto return_from_cache;
	}

	cache.is_xsave_cached = 1;
	cache.is_xsave_available = is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_ECX_XSAVE, &(info.ECX));

return_from_cache:
	return cache.is_xsave_available;
}

unsigned char is_osxsave_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_ECX_OSXSAVE, &(info.ECX));
}

unsigned char is_avx_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_ECX_AVX, &(info.ECX));
}

unsigned char is_fpu_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_EDX_FPU, &(info.EDX));
}


unsigned char is_fxsr_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_EDX_FXSR, &(info.EDX));
}

unsigned char is_sse_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_FEATURE_INFO, 0x0, X86_CPUID1_EDX_SSE, &(info.EDX));
}

unsigned char is_fsgsbase_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_STRUCTURED_EXTENDED_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_STRUCTURED_EXTENDED_FEATURE_INFO, 0x0, X86_CPUID7_EBX_FSGSBASE, &(info.EBX));
}

unsigned char is_pku_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_STRUCTURED_EXTENDED_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_STRUCTURED_EXTENDED_FEATURE_INFO, 0x0, X86_CPUID7_ECX_PKU, &(info.ECX));
}

unsigned char is_ospke_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_STRUCTURED_EXTENDED_FEATURE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_STRUCTURED_EXTENDED_FEATURE_INFO, 0x0, X86_CPUID7_ECX_OSPKE, &(info.ECX));
}

unsigned char is_xsaveopt_available(void)
{
	// TODO cache the response ??

	if (__highest_basic_func_value < EAX_EXTENDED_STATE_INFO) {
		return 0;
	}

	return is_flag_enabled(EAX_EXTENDED_STATE_INFO, 0x1, X86_CPUIDD1_EAX_XSAVEOPT, &(info.EAX));
}


unsigned char is_syscall_available(void)
{
	// TODO cache the response ??

	if (__highest_extended_func_value < EAX_EXTENDED_PROCESSOR_INFO_AND_FUTURE_BITS) {
		return 0;
	}

	return is_flag_enabled(EAX_EXTENDED_PROCESSOR_INFO_AND_FUTURE_BITS, 0x0, X86_CPUID3_SYSCALL, &(info.EDX));
}


// this function will be able to be called from outside for general queries
unsigned char get_cpuid_info(unsigned int primary_code, unsigned int secondary_code)
{
	// TODO implement
}
