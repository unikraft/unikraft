#include <procfs/proc.h>
#include <vfscore/vnode.h>
#include <string.h>
#include <uk/store.h>


#ifdef CONFIG_LIBPROCFS_MOUNTS
static char *mounts_name = "mounts";

static int mounts_read(struct procfs_entry *entry __unused, struct uio *uio,
			   int flags __unused)
{
	const struct uk_store_entry *store_entry;
	int rc;
	char *buff;
	store_entry = uk_store_static_entry_get(uk_libid("libvfscore"), 0x02);
	if (!store_entry) {
		uk_pr_crit("Failed to get mounts entry from store\n");
		return -ENOENT;
	}
	rc = uk_store_get_value(store_entry, charp, &buff);
	vfscore_uiomove(buff, strlen(buff), uio);
	return 0;
}

static struct procops mounts_procops = {
	.read = mounts_read,
	.write = proc_noop_write,
	.readlink = proc_noop_readlink,
};

#endif /* CONFIG_LIBPROCFS_MOUNTS */

int procfs_register_mounts()
{
	int rc = 0;

#ifdef CONFIG_LIBPROCFS_MOUNTS
	uk_pr_debug("Register '%s' to procfs\n", mounts_name);

	/* register /proc/mounts */
	rc = procfs_create_entry(mounts_name, PROCFS_NODE_FILE, &mounts_procops);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to procfs: %d\n",
			  mounts_name, rc);
		return -rc;
	}
#endif /* CONFIG_LIBPROCFS_MOUNTS */

	return rc;
}
