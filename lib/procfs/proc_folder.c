#include <procfs/proc.h>
#include <vfscore/vnode.h>
#include <string.h>
#include <uk/store.h>



#ifdef CONFIG_LIBPROCFS_PROC_FOLDER
#define CONVERSION_BASE 10

static int proc_noop_read(struct procfs_entry *entry __unused, struct uio *uio,
			  int flags __unused)
{
	/* No-op read function */
	return vfscore_uiomove("", 0, uio);
}

static int convert(int n, char str[], int i) {
	if (n/CONVERSION_BASE > 0)
		i = convert(n/CONVERSION_BASE, str, i);
	str[i++] = "0123456789"[n%CONVERSION_BASE];
	return i;
}

static int itoa(int n, char str[]) {
	int i = convert(n, str, 0);
	str[i] = '\0';
	return i;
}

static struct procops cgroup_procops = {
    .read = proc_noop_read,
    .write = proc_noop_write,
    .readlink = proc_noop_readlink,
};

static struct procops maps_procops = {
	.read =proc_noop_read,
	.write = proc_noop_write,
    .readlink = proc_noop_readlink,
};

static struct procops arch_status_procops = {
	.read =proc_noop_read,
	.write = proc_noop_write,
    .readlink = proc_noop_readlink,
};

#endif /* CONFIG_LIBPROCFS_PROC_FOLDER */

int procfs_register_proc_folder(size_t id)
{
	int rc = 0;
    char folder_id[10], buffer[NAME_MAX];
    memset(buffer, 0, NAME_MAX);
    itoa(id, folder_id);
#ifdef CONFIG_LIBPROCFS_PROC_FOLDER
	uk_pr_debug("Register '%s' to procfs\n", folder_id);

	rc = procfs_create_entry(folder_id, PROCFS_NODE_DIR, NULL);

    strcat(buffer, folder_id);
    strcat(buffer, "/arch_status");
    rc = procfs_create_entry(buffer, PROCFS_NODE_FILE, &arch_status_procops);

    memset(buffer, 0, NAME_MAX);
    strcat(buffer, folder_id);
    strcat(buffer, "/cgroup");
    rc = procfs_create_entry(buffer, PROCFS_NODE_FILE, &cgroup_procops);

    memset(buffer, 0, NAME_MAX);
    strcat(buffer, folder_id);
    strcat(buffer, "/maps");
    rc = procfs_create_entry(buffer, PROCFS_NODE_FILE, &maps_procops);

	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to procfs: %d\n",
			  folder_id, rc);
		return -rc;
	}
#endif /* CONFIG_LIBPROCFS_PROC_FOLDER */

	return rc;
}
