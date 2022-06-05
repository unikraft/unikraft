#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uk/swrand.h>
#include <uk/print.h>
#include <uk/hwrand.h>

ssize_t uk_hwrand_generate_bytes(void *buf, size_t buflen)
{
	size_t idx = 0, rem = buflen;
	size_t safety = buflen / sizeof(unsigned int);
	u_int8_t success;
	__u32 val;

	while (rem > 0 && safety > 0) {
		success = uk_hwrand_randr(&val);

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
