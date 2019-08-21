#include <uk/version.h>
#include <uk/essentials.h>
#include <stdio.h>

void uk_version(void)
{
	printf("Unikraft "
		STRINGIFY(UK_CODENAME) " "
		STRINGIFY(UK_FULLVERSION) "\n");
}

