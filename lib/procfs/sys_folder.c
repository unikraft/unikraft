#include <procfs/proc.h>
#include <vfscore/vnode.h>
#include <string.h>
#include <uk/store.h>



#ifdef CONFIG_LIBPROCFS_SYS_FOLDER

char* overcommit_memory = "sys/vm/overcommit_memory";
char* somaxconn = "sys/net/core/somaxconn";


static int proc_noop_read(struct procfs_entry *entry __unused, struct uio *uio,
			  int flags __unused)
{
	/* No-op read function */
	return vfscore_uiomove("", 0, uio);
}

static struct procops overcommit_memory_procops = {
	.read =proc_noop_read,
	.write = proc_noop_write,
    .readlink = proc_noop_readlink
};

static struct procops somaxconn_procops = {
	.read =proc_noop_read,
	.write = proc_noop_write,
    .readlink = proc_noop_readlink
};
#endif /* CONFIG_LIBPROCFS_SYS_FOLDER */

int procfs_register_sys_folder()
{
	int rc = 0;
#ifdef CONFIG_LIBPROCFS_SYS_FOLDER
	uk_pr_debug("Register '%s' to procfs\n", overcommit_memory);
	rc = procfs_create_entry(overcommit_memory, PROCFS_NODE_FILE, &overcommit_memory_procops);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to procfs: %d\n",
			  overcommit_memory, rc);
		return -rc;
	}

    uk_pr_debug("Register '%s' to procfs\n", somaxconn);
	rc = procfs_create_entry(somaxconn, PROCFS_NODE_FILE, &somaxconn_procops);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to procfs: %d\n",
			  somaxconn, rc);
		return -rc;
	}
#endif /* CONFIG_LIBPROCFS_SYS_FOLDER */

	return rc;
}
