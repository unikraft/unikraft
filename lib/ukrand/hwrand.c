#include <stdlib.h>
#include <string.h>
#include <uk/rdrand.h>
#include <uk/entropy.h>
#include <uk/cpuid.h>
#include <uk/print.h>

size_t uk_hwrand_generate_bytes(void *buf, size_t buflen) {
	size_t offset = uk_entropy_generate_bytes(buf, buflen);
	uk_pr_crit("HWRAND ENTROPY = %ld\n", offset);
	
	if (offset < buflen) {
		if (is_RDRAND_available()) {
			uk_pr_crit("HWRAND RDRAND = %ld\n", buflen - offset);
			offset += uk_rdrand_generate_bytes(buf + offset, buflen - offset);
		}
	}

	return offset;
}

__u32 uk_hw_get_estimated_entropy(void) {
	return uk_entropy_get_estimated_entropy();
}

int _uk_hwrand_init(void)
{	
	return uk_entropy_init();
}
