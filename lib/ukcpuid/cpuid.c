#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <uk/errptr.h>
#include <uk/cpuid.h>


UK_LIST_HEAD(uk_cpuid_list);


int get_cpuid_info(struct uk_cpuid_info *info, unsigned int primary_code, unsigned int secondary_code)
{
	struct uk_cached_cpuid_entry *cpuid_entry, *cpuid_entry_next;
	bool info_found = 0;

	uk_list_for_each_entry_safe(cpuid_entry, cpuid_entry_next, &uk_cpuid_list, list) {
	if (cpuid_entry->primary_code == primary_code &&
		cpuid_entry->secondary_code == secondary_code) {
		info->EAX = cpuid_entry->info.EAX;
		info->EBX = cpuid_entry->info.EBX;
		info->ECX = cpuid_entry->info.ECX;
		info->EDX = cpuid_entry->info.EDX;
		info_found = 1;
		break;
	}
}

	if (!info_found) {
		__asm__ __volatile__ (
		"cpuid"
		: "=a"(info->EAX), "=b"(info->EBX),
		  "=c"(info->ECX), "=d"(info->EDX)
		: "a"(primary_code), "c"(secondary_code)
		);

		cpuid_entry = malloc(sizeof(struct uk_cached_cpuid_entry));
		if (!cpuid_entry) {
			return -1;
		}

		cpuid_entry->info = *info;
		uk_list_add(&cpuid_entry->list, &uk_cpuid_list);
	}

	return 0;
}
