#include <uk/version.h>
#include <uk/essentials.h>
#include <stdio.h>
#ifdef CONFIG_LIBPROCFS_VERSION
#include <stdlib.h>
#include <uk/store.h>
#endif /* CONFIG_LIBPROCFS_VERSION */


void uk_version(void)
{
	printf("Unikraft "
		STRINGIFY(UK_CODENAME) " "
		STRINGIFY(UK_FULLVERSION) "\n");
}

#ifdef CONFIG_LIBPROCFS_VERSION
static int get_uk_version(void *cookie __unused, char **out)
{
	*out = strdup(
			"Unikraft "
			STRINGIFY(UK_CODENAME) " "
			STRINGIFY(UK_FULLVERSION) "\n"
			);
	
	if (*out == NULL)
		return -ENOMEM;
	
	return 0;
}											
UK_STORE_STATIC_ENTRY(uk_version, charp, get_uk_version, NULL, NULL);
#endif /* CONFIG_LIBPROCFS_VERSION */
