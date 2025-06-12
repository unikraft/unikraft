#include <procfs/proc.h>
#include <vfscore/vnode.h>
#include <string.h>
#include <uk/store.h>


#ifdef CONFIG_LIBPROCFS_VERSION
static char *version_name = "version";

static int version_read(struct procfs_entry *entry __unused, struct uio *uio,
			   int flags __unused)
{
	const struct uk_store_entry *store_entry;
	int rc;
	char *buff;
	__u16 lib_id = uk_libid("libukboot");
	store_entry = uk_store_static_entry_get(lib_id, 0x01);
	if (!store_entry) {
		uk_pr_debug("Failed to get version entry from store\n");
		return -ENOENT;
	}
	rc = uk_store_get_value(store_entry, charp, &buff);
	vfscore_uiomove(buff, strlen(buff), uio);
	return 0;
}

static struct procops version_procops = {
	.read = version_read,
	.write = proc_noop_write,
	.readlink = proc_noop_readlink,
};

#endif /* CONFIG_LIBPROCFS_VERSION */

int procfs_register_version()
{
	int rc = 0;

#ifdef CONFIG_LIBPROCFS_VERSION
	uk_pr_debug("Register '%s' to procfs\n", version_name);

	/* register /proc/version */
	rc = procfs_create_entry(version_name, PROCFS_NODE_FILE, &version_procops);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to procfs: %d\n",
			  version_name, rc);
		return -rc;
	}
#endif /* CONFIG_LIBPROCFS_VERSION */

	return rc;
}
