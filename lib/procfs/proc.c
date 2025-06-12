#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#include <errno.h>
#include <uk/mutex.h>
#include <string.h>
#include <stdlib.h>

#include <vfscore/file.h>

#include <procfs/proc.h>

struct procfs_entry *entries, *last_entry;
struct uk_mutex procfs_lock = UK_MUTEX_INITIALIZER(procfs_lock);

void
set_times_to_now(struct timespec *time1, struct timespec *time2,
		 struct timespec *time3)
{
	struct timespec now = {0, 0};

	/* TODO: implement the real clock_gettime */
	/* clock_gettime(CLOCK_REALTIME, &now); */
	if (time1)
		memcpy(time1, &now, sizeof(struct timespec));
	if (time2)
		memcpy(time2, &now, sizeof(struct timespec));
	if (time3)
		memcpy(time3, &now, sizeof(struct timespec));
}

int proc_link_read(struct procfs_entry *proc, struct uio *uio)
{
    struct procops *ops;
    int error;

    ops = proc->ops;
    UK_ASSERT(ops->readlink != NULL);
    error = (*ops->readlink)(proc, uio);

    return error;
}
int proc_file_read(struct procfs_entry *proc, struct uio *uio, int ioflags)
{
	struct procops *ops;
	int error;
    
    uk_pr_debug("proc_file_read: %s\n", proc->name);
	ops = proc->ops;
	UK_ASSERT(ops->read != NULL);
	error = (*ops->read)(proc, uio, ioflags);

	return error;
}

int proc_file_readdir(struct procfs_entry *proc, struct vfscore_file *fp,
	      struct dirent64 *dir)
{
    struct procfs_entry *np, *dnp;
	int i;

	uk_mutex_lock(&procfs_lock);

	set_times_to_now(&(proc->rn_atime),NULL, NULL);

	if (fp->f_offset == 0) {
		dir->d_type = DT_DIR;
		strlcpy((char *) &dir->d_name, ".", sizeof(dir->d_name));
	} else if (fp->f_offset == 1) {
		dir->d_type = DT_DIR;
		strlcpy((char *) &dir->d_name, "..", sizeof(dir->d_name));
	} else {
		dnp = proc;
		np = dnp->children;
		if (np == NULL) {
			uk_mutex_unlock(&procfs_lock);
			return ENOENT;
		}

		for (i = 0; i != (fp->f_offset - 2); i++) {
			np = np->sibling;
			if (np == NULL) {
				uk_mutex_unlock(&procfs_lock);
				return ENOENT;
			}
		}
		if (np->type == PROCFS_NODE_DIR)
			dir->d_type = DT_DIR;
		else if (np->type == PROCFS_NODE_LNK)
			dir->d_type = DT_LNK;
		else
			dir->d_type = DT_REG;
		strlcpy((char *) &dir->d_name, np->name,
				sizeof(dir->d_name));
	}
	dir->d_fileno = fp->f_offset;

	fp->f_offset++;

	uk_mutex_unlock(&procfs_lock);

	return 0;
}

static struct procfs_entry *create_node(char* name, procfs_node_type_t type, struct procops *ops)
{
    struct procfs_entry *node;

    node = malloc(sizeof(struct procfs_entry));
    if (node == NULL) {
        return NULL;
    }

    node->name = strdup(name);
    node->name_len = strlen(name);
    if (node->name == NULL) {
        free(node);
        return NULL;
    }

    node->type = type;
    if (type == PROCFS_NODE_DIR)
		node->mode = S_IFDIR|0777;
	else if (type == PROCFS_NODE_LNK)
		node->mode = S_IFLNK|0777;
	else
		node->mode = S_IFREG|0777;
    node->ops = ops;

    node->children = NULL;
    node->sibling = NULL;
    node->next = NULL;
    node->parent = NULL;
    node->rn_size = 0;

    set_times_to_now(&(node->rn_ctime), &(node->rn_atime), &(node->rn_mtime));

    node->rn_owns_buffer = true;

    return node;
}

static struct procfs_entry *check_existence(char *name, struct procfs_entry *parent)
{
    struct procfs_entry *cur = entries;

    while (cur != NULL) {
        if (!strcmp(cur->name, name) && cur->parent == parent)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

static void print_names(){
    struct procfs_entry *cur = entries;

    while (cur != NULL) {
        uk_pr_debug("%s ", cur->name);
        cur = cur->next;
    }
}

static int insert_node(struct procfs_entry *parent, struct procfs_entry *node)
{
    struct procfs_entry *cur;

    if (parent->children == NULL) {
        parent->children = node;
        last_entry->next = node;
        last_entry = node;
        node->parent = parent;
    } else {
        cur = parent->children;
        while (cur->sibling != NULL) {
            cur = cur->sibling;
        }
        cur->sibling = node;
        last_entry->next = node;
        last_entry = node;
        node->parent = parent;
    }
    return 0;
}

int
procfs_create_entry(char *name, procfs_node_type_t type, struct procops *ops)
{
	struct procfs_entry *np, *cur_root;
    char* tmp_1, *tmp_2;

	if (name && strlen(name) > NAME_MAX)
		return ENAMETOOLONG;

    cur_root = entries;
    tmp_1 = strchr(name, '/');
    tmp_2 = name;
    
    while (tmp_1)
    {
        *tmp_1 = '\0';
        if (check_existence(tmp_2, cur_root)) {
            cur_root = check_existence(tmp_2, cur_root);
            tmp_2 = tmp_1 + 1;
            tmp_1 = strchr(tmp_2, '/');
            continue;
        }
        else{
            np = create_node(tmp_2, PROCFS_NODE_DIR, NULL);
            if (np == NULL)
                return -ENOMEM;
            insert_node(cur_root, np);
            cur_root = np;
        }
        
        tmp_2 = tmp_1 + 1;
        tmp_1 = strchr(tmp_2, '/');
    }
    if (!check_existence(tmp_2, cur_root)) {
        np = create_node(tmp_2, type, ops);
        if (np == NULL)
            return -ENOMEM;
        insert_node(cur_root, np);
    }
    
    return 0;
}

int create_root(struct mount *mp)
{
	entries = create_node("/", PROCFS_NODE_DIR, NULL);
    last_entry = entries;
	if (entries == NULL)
		return -ENOMEM;

    mp->m_root->d_vnode->v_data = entries;
	return 0;
}

int
procop_noop()
{
	return 0;
}

int
procop_eperm()
{
	return EPERM;
}
