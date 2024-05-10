#include <stubs.h>
#include <string.h>

void explicit_bzero(void *p, size_t n)
{
    volatile unsigned char *vp = p;
    while (n--) {
        *vp++ = 0;
    }
    /* Add a barrier to prevent the compiler from optimizing out the loop,
	 * similar to the one in the original code. */
	__asm__ __volatile__("" : : "r"(p) : "memory");
}

int
timingsafe_bcmp(const void *b1, const void *b2, size_t n)
{
	const unsigned char *p1 = b1, *p2 = b2;
	int ret = 0;

	for (; n > 0; n--)
		ret |= *p1++ ^ *p2++;
	return (ret != 0);
}
