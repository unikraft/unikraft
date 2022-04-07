#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uk/swrand.h>
#include "rdrand.h"

int has_amd_cpu(void)
{
	struct CPUID_info info;

	cpuid_macro(info.EAX, info.EBX, info.ECX, info.EDX, 0x00, 0x00);

	return (memcmp((char *) (&info.EBX), "htuA", 4) == 0
		&& memcmp((char *) (&info.EDX), "itne", 4) == 0
		&& memcmp((char *) (&info.ECX), "DMAc", 4) == 0);
}


int has_intel_cpu(void)
{
	struct CPUID_info info;

	cpuid_macro(info.EAX, info.EBX, info.ECX, info.EDX, 0x00, 0x00);

	return (memcmp((char *) (&info.EBX), "Genu", 4) == 0
		&& memcmp((char *) (&info.EDX), "ineI", 4) == 0
		&& memcmp((char *) (&info.ECX), "ntel", 4) == 0);
}

int has_RDRAND(void)
{
	struct CPUID_info info;
	static const unsigned int RDRAND_FLAG = (1 << 30);

	if (!(has_amd_cpu() || has_intel_cpu())) {
		return 0;
	}

	cpuid_macro(info.EAX, info.EBX, info.ECX, info.EDX, 0x00, 0x00);

	return (info.ECX & RDRAND_FLAG) == RDRAND_FLAG;
}

int RDRAND_bytes(unsigned char *buf, size_t buflen)
{
	size_t idx = 0, rem = buflen;
	size_t safety = buflen / sizeof(unsigned int) + 4;
	u_int8_t error_code;
	unsigned int val;

	if (!has_RDRAND()) {
		return -1;
	}

	while (rem > 0 && safety > 0) {
		__asm__ volatile(
			"rdrand %0 ; setc %1"
			: "=r" (val), "=qm" (error_code)
		);

		if (error_code) {
			size_t cnt = (rem < sizeof(val) ? rem : sizeof(val));

			memcpy(buf + idx, &val, cnt);

			rem -= cnt;
			idx += cnt;
		} else {
			safety--;
		}
	}

	*((volatile unsigned int*)&val) = 0;

	return (ssize_t)(buflen - rem);
}
