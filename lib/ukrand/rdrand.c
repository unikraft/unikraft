#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uk/rdrand.h>

size_t uk_rdrand_generate_bytes(void *buf, size_t buflen)
{
	size_t idx = 0, rem = buflen;
	size_t safety = buflen / sizeof(unsigned int);
	__u8 success;
	__u32 val;

	while (rem > 0 && safety > 0) {
		success = uk_hwrand_rdrand(&val);

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

	return (size_t)(buflen - rem);
}
