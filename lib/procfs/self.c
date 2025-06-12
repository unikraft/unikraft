#include <procfs/proc.h>
#include <vfscore/vnode.h>
#include <string.h>
#include <uk/store.h>



#ifdef CONFIG_LIBPROCFS_SELF_LINK
const char *procfs_self_name = "self";


static int version_read(struct procfs_entry *entry __unused, struct uio *uio,
			   int flags __unused)
{
	return 0;
}

static int readlink_self(struct procfs_entry *entry, struct uio *uio)
{
    const struct uk_store_entry *store_entry;
	int rc;
	char *buff;
	__u16 lib_id = uk_libid("libposix_process");
	store_entry = uk_store_static_entry_get(lib_id, 0x01);
	if (!store_entry) {
		uk_pr_crit("Failed to get id entry from store\n");
		return -ENOENT;
	}
	rc = uk_store_get_value(store_entry, charp, &buff);
    
    return vfscore_uiomove(buff, strlen(buff), uio);
}

static struct procops self_procops = {
	.read = version_read,
	.write = proc_noop_write,
    .readlink = readlink_self,
};

#endif /* CONFIG_LIBPROCFS_SELF_LINK */

int procfs_register_self_link()
{
	int rc = 0;
#ifdef CONFIG_LIBPROCFS_SELF_LINK
	uk_pr_debug("Register '%s' to procfs\n", procfs_self_name);

	/* register /proc/version */
    rc = procfs_create_entry(procfs_self_name, PROCFS_NODE_LNK, &self_procops);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to procfs: %d\n",
			  procfs_self_name, rc);
		return -rc;
	}
#endif /* CONFIG_LIBPROCFS_SELF_LINK */

	return rc;
}
