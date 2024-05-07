#include <stubs.h>
#include <string.h>

void explicit_bzero(void *p, size_t n)
{
    /* TODO: Replace this with the freebsd implementation */
    volatile unsigned char *vp = p;
    while (n--) {
        *vp++ = 0;
    }
}
