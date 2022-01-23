#ifndef __UK_CPUID_H__
#define __UK_CPUID_H__

#include <uk/list.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct uk_list_head uk_cpuid_list;

struct uk_cpuid_info {
	unsigned int EAX;
	unsigned int EBX;
	unsigned int ECX;
	unsigned int EDX;
};

struct uk_cached_cpuid_entry {
	struct uk_list_head list;
	unsigned int primary_code;
	unsigned int secondary_code;
	struct uk_cpuid_info info;
};

int get_cpuid_info(struct uk_cpuid_info *info, unsigned int primary_code,
					unsigned int secondary_code);

#ifdef __cplusplus
}
#endif

#endif
