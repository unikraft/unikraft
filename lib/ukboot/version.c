#include <uk/version.h>
#include <uk/essentials.h>
#include <stdio.h>

void uk_version(void)
{
	printf("Unikraft "
		STRINGIFY(UK_CODENAME) " "
		STRINGIFY(UK_FULLVERSION) "\n");
}

#ifdef CONFIG_LIBPROCFS_VERSION
#include <uk/store.h>
#include <uk/boot/store.h>

static int get_uk_version(void *cookie __unused, char **out)
{
	*out = strdup(
			"Unikraft "
			STRINGIFY(UK_CODENAME) " "
			STRINGIFY(UK_FULLVERSION)
			);

	if (*out == NULL)
		return -ENOMEM;

	return 0;
}											
UK_STORE_STATIC_ENTRY(UK_BOOT_VERSION, uk_version_global, charp, get_uk_version, NULL);
#endif /* CONFIG_LIBPROCFS_VERSION */
