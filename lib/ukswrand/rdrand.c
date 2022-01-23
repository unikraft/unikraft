#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uk/swrand.h>
#include <uk/cpuid.h>


int has_amd_cpu(void)
{
	struct CPUIDinfo info;

	if (get_cpuid_info(&info, 0x00, 0x00)) {
		return 0;
	}

	return (memcmp((char *) (&info.EBX), "htuA", 4) == 0
		&& memcmp((char *) (&info.EDX), "itne", 4) == 0
		&& memcmp((char *) (&info.ECX), "DMAc", 4) == 0);
}


int has_intel_cpu(void)
{
	struct CPUIDinfo info;

	if (get_cpuid_info(&info, 0x00, 0x00)) {
		return 0;
	}

	return (memcmp((char *) (&info.EBX), "Genu", 4) == 0
		&& memcmp((char *) (&info.EDX), "ineI", 4) == 0
		&& memcmp((char *) (&info.ECX), "ntel", 4) == 0);
}

int has_RDRAND(void)
{
	struct CPUIDinfo info;
	static const unsigned int RDRAND_FLAG = (1 << 30);

	if (!(has_amd_cpu() || has_intel_cpu())) {
		return 0;
	}

	if (get_cpuid_info(&info, 0x01, 0x00)) {
		return 0;
	}

	return (info.ECX & RDRAND_FLAG) == RDRAND_FLAG;
}

ssize_t RDRAND_bytes(unsigned char *buf, size_t buflen)
{
	if (!has_RDRAND()) {
		return -1;
	}

	size_t idx = 0, rem = buflen;
	size_t safety = buflen / sizeof(unsigned int) + 4;

	unsigned int val;

	while (rem > 0 && safety > 0) {
		char rc;

		__asm__ volatile(
			"rdrand %0 ; setc %1"
			: "=r" (val), "=qm" (rc)
		);

		if (rc) {
			size_t cnt = (rem < sizeof(val) ? rem : sizeof(val));

			memcpy(buf + idx, &val, cnt);

			rem -= cnt;
			idx += cnt;
		} else {
			safety--;
		}
	}

	*((volatile unsigned int*)&val) = 0;

	return (ssize_t)rem;
}
