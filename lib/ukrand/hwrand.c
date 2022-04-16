#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uk/swrand.h>
#include <uk/print.h>
#include <uk/hwrand.h>


// TODO replace below calls with libukcpuid requests
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

ssize_t uk_hwrand_generate_bytes(void *buf, size_t buflen)
{
	size_t idx = 0, rem = buflen;
	size_t safety = buflen / sizeof(unsigned int);
	u_int8_t success;
	__u32 val;

	while (rem > 0 && safety > 0) {
		val = uk_hwrand_randr(&success);

		if (success) {
			size_t cnt = (rem < sizeof(val) ? rem : sizeof(val));

			memcpy((unsigned char*)buf + idx, &val, cnt);

			rem -= cnt;
			idx += cnt;
		} else {
			safety--;
		}
	}

	*((volatile unsigned int*)&val) = 0;

	return (ssize_t)(buflen - rem);
}
