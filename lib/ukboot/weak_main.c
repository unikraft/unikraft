#include <stdio.h>
#include <errno.h>
#include <uk/essentials.h>

/* Internal main */
int __weak main(int argc __unused, char *argv[] __unused)
{
	printf("weak main() called. Symbol was not replaced!\n");
	return -EINVAL;
}
