#include <stdlib.h>
#include <string.h>
#include <uk/rdrand.h>
#include <uk/entropy.h>
#include <uk/cpuid.h>
#include <uk/print.h>

size_t uk_hwrand_generate_bytes(void *buf, size_t buflen) {
	size_t offset = uk_entropy_generate_bytes(buf, buflen);
	
	if (offset < buflen) {
		if (is_RDRAND_available()) {
			offset += uk_rdrand_generate_bytes(buf + offset, buflen - offset);
		}
	}

	return offset;
}

int _uk_hwrand_init(void)
{	
	return uk_entropy_init();
}
