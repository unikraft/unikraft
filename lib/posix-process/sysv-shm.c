/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <limits.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/errptr.h>
#include <uk/list.h>
#include <uk/plat/time.h>
#include <uk/process.h>
#include <uk/spinlock.h>
#include <uk/syscall.h>
#include <uk/vmem.h>
#include <uk/vmem/vma_types.h>

#define IPC_PRIVATE	0
#define IPC_CREAT	01000
#define IPC_EXCL	02000
#define IPC_RMID	0
#define IPC_STAT	2
#define IPC_64		0x100

#define SHM_DEST	01000
#define SHM_RDONLY	010000
#define SHM_RND		020000

#define SHM_MODE_MASK	0777
#define SHM_READ	4
#define SHM_WRITE	2

/* Native Linux LP64 IPC_STAT layout. The syscall copies this representation
 * regardless of the headers used to build the kernel.
 */
struct shm_stat {
	struct {
		__s32 key;
		__u32 uid;
		__u32 gid;
		__u32 cuid;
		__u32 cgid;
		__u32 mode;
		__u16 sequence;
		__u16 pad;
		__u64 reserved[2];
	} perm;
	__u64 size;
	__s64 atime;
	__s64 dtime;
	__s64 ctime;
	__s32 cpid;
	__s32 lpid;
	__u64 nattch;
	__u64 reserved[2];
};

UK_CTASSERT(__offsetof(struct shm_stat, size) == 48);
UK_CTASSERT(__offsetof(struct shm_stat, nattch) == 88);
UK_CTASSERT(sizeof(struct shm_stat) == 112);

struct shm_segment {
	struct uk_list_head link;
	struct uk_alloc *alloc;
	struct uk_vma_shared_anon *backing;
	struct shm_stat stat;
	unsigned int attaching;
	int id;
};

static uk_spinlock shm_lock = UK_SPINLOCK_INITIALIZER();
static UK_LIST_HEAD(shm_segments);
static int shm_next_id;

static struct shm_segment *shm_find_id(int id)
{
	struct shm_segment *s;

	uk_list_for_each_entry(s, &shm_segments, link)
		if (s->id == id)
			return s;
	return NULL;
}

static struct shm_segment *shm_find_key(int key)
{
	struct shm_segment *s;

	uk_list_for_each_entry(s, &shm_segments, link)
		if (s->stat.perm.key == key)
			return s;
	return NULL;
}

static __s64 shm_time(void)
{
	return ukarch_time_nsec_to_sec(ukplat_wall_clock());
}

static void shm_free(struct shm_segment *s)
{
	uk_vma_shared_anon_release(s->backing);
	uk_free(s->alloc, s);
}

/* The caller holds shm_lock. The backing is freed outside that lock. */
static bool shm_unlink_if_unused(struct shm_segment *s)
{
	if (!(s->stat.perm.mode & SHM_DEST) || s->stat.nattch || s->attaching)
		return false;
	uk_list_del(&s->link);
	return true;
}

static void shm_attach(void *arg)
{
	struct shm_segment *s = arg;
	unsigned long irqf;

	uk_spin_lock_irqsave(&shm_lock, irqf);
	s->stat.nattch++;
	uk_spin_unlock_irqrestore(&shm_lock, irqf);
}

static void shm_detach(void *arg)
{
	struct shm_segment *s = arg;
	__s64 now = shm_time();
	int pid = uk_sys_getpid();
	unsigned long irqf;
	bool free_segment;

	uk_spin_lock_irqsave(&shm_lock, irqf);
	UK_ASSERT(s->stat.nattch);
	s->stat.nattch--;
	s->stat.dtime = now;
	if (pid >= 0)
		s->stat.lpid = pid;
	free_segment = shm_unlink_if_unused(s);
	uk_spin_unlock_irqrestore(&shm_lock, irqf);
	if (free_segment)
		shm_free(s);
}

static bool shm_allowed(const struct shm_segment *s, unsigned int access)
{
	/* libposix-user exposes the same effective credentials in all tasks. */
	return s->stat.perm.uid == 0 ||
	       (((s->stat.perm.mode >> 6) & access) == access);
}

static int shm_get_existing(const struct shm_segment *s, size_t size, int flags)
{
	unsigned int access = (flags | (flags >> 3) | (flags >> 6)) & 7;

	if ((flags & (IPC_CREAT | IPC_EXCL)) == (IPC_CREAT | IPC_EXCL))
		return -EEXIST;
	if (size > s->stat.size)
		return -EINVAL;
	if (!shm_allowed(s, access))
		return -EACCES;
	return s->id;
}

UK_SYSCALL_R_DEFINE(int, shmget, int, key, size_t, size, int, flags)
{
	struct uk_vas *vas = uk_vas_get_active();
	struct shm_segment *existing, *s;
	unsigned long irqf;
	int rc;

	if (!vas || !uk_vas_is_owned(vas))
		return -ENOTSUP;
	if (flags & ~(IPC_CREAT | IPC_EXCL | SHM_MODE_MASK))
		return -EINVAL;
	if (key != IPC_PRIVATE) {
		uk_spin_lock_irqsave(&shm_lock, irqf);
		existing = shm_find_key(key);
		rc = existing ? shm_get_existing(existing, size, flags)
			      : -ENOENT;
		uk_spin_unlock_irqrestore(&shm_lock, irqf);
		if (rc != -ENOENT || !(flags & IPC_CREAT))
			return rc;
	}
	if (!size || size > __SZ_MAX - (UK_PAGING_PAGE_SIZE - 1))
		return -EINVAL;

	s = uk_calloc(vas->a, 1, sizeof(*s));
	if (!s)
		return -ENOMEM;
	s->alloc = vas->a;
	rc = uk_vma_shared_anon_create(vas, UK_PAGING_PAGE_ALIGN_UP(size),
				       &s->backing);
	if (rc) {
		uk_free(s->alloc, s);
		return rc;
	}
	s->stat.perm.key = key;
	s->stat.perm.uid = CONFIG_LIBPOSIX_USER_UID;
	s->stat.perm.gid = CONFIG_LIBPOSIX_USER_GID;
	s->stat.perm.cuid = s->stat.perm.uid;
	s->stat.perm.cgid = s->stat.perm.gid;
	s->stat.perm.mode = flags & SHM_MODE_MASK;
	s->stat.size = size;
	s->stat.ctime = shm_time();
	s->stat.cpid = uk_sys_getpid();

	uk_spin_lock_irqsave(&shm_lock, irqf);
	existing = key == IPC_PRIVATE ? NULL : shm_find_key(key);
	if (existing) {
		rc = shm_get_existing(existing, size, flags);
		uk_spin_unlock_irqrestore(&shm_lock, irqf);
		shm_free(s);
		return rc;
	}
	do {
		s->id = shm_next_id;
		shm_next_id = s->id == INT_MAX ? 0 : s->id + 1;
	} while (shm_find_id(s->id));
	rc = s->id;
	uk_list_add_tail(&s->link, &shm_segments);
	uk_spin_unlock_irqrestore(&shm_lock, irqf);
	return rc;
}

UK_SYSCALL_R_DEFINE(void *, shmat, int, id, const void *, addr, int, flags)
{
	struct uk_vas *vas = uk_vas_get_active();
	struct uk_vma_shared_anon_attachment attachment;
	struct shm_segment *s;
	unsigned long attr = UK_PAGING_PAGE_ATTR_PROT_READ;
	unsigned int access = SHM_READ;
	__vaddr_t vaddr = UK_PAGING_VADDR_ANY;
	unsigned long irqf;
	__s64 now;
	bool free_segment;
	int rc;

	if (!vas || !uk_vas_is_owned(vas))
		return ERR2PTR(-ENOTSUP);
	if (flags & ~(SHM_RDONLY | SHM_RND))
		return ERR2PTR(-EINVAL);
	if (addr) {
		vaddr = (__vaddr_t)addr;
		if (flags & SHM_RND)
			vaddr = UK_PAGING_PAGE_ALIGN_DOWN(vaddr);
		if (!UK_PAGING_PAGE_ALIGNED(vaddr) ||
		    vaddr < vas->range_start || vaddr >= vas->range_end)
			return ERR2PTR(-EINVAL);
	}
	if (!(flags & SHM_RDONLY)) {
		access |= SHM_WRITE;
		attr |= UK_PAGING_PAGE_ATTR_PROT_WRITE;
	}

	/* Keep the segment alive across uk_vma_map without nesting a VAS lock
	 * inside shm_lock. IPC_RMID can run between lookup and mapping.
	 */
	uk_spin_lock_irqsave(&shm_lock, irqf);
	s = shm_find_id(id);
	if (!s) {
		rc = -EINVAL;
	} else if (!shm_allowed(s, access)) {
		rc = -EACCES;
	} else {
		s->attaching++;
		rc = 0;
	}
	uk_spin_unlock_irqrestore(&shm_lock, irqf);
	if (rc)
		return ERR2PTR(rc);

	attachment = (struct uk_vma_shared_anon_attachment) {
		.backing = s->backing,
		.attach = shm_attach,
		.detach = shm_detach,
		.arg = s,
		.readonly = !!(flags & SHM_RDONLY),
	};
	rc = uk_vma_map(vas, &vaddr, UK_PAGING_PAGE_ALIGN_UP(s->stat.size),
			attr, UK_VMA_MAP_SIZE_4KB, NULL,
			&uk_vma_shared_anon_attachment_ops, &attachment);
	now = shm_time();

	uk_spin_lock_irqsave(&shm_lock, irqf);
	s->attaching--;
	if (!rc) {
		s->stat.atime = now;
		s->stat.lpid = uk_sys_getpid();
	}
	free_segment = shm_unlink_if_unused(s);
	uk_spin_unlock_irqrestore(&shm_lock, irqf);
	if (free_segment)
		shm_free(s);

	if (rc)
		return ERR2PTR(rc == -EEXIST ? -EINVAL : rc);
	return (void *)vaddr;
}

UK_SYSCALL_R_DEFINE(int, shmdt, const void *, addr)
{
	struct uk_vas *vas = uk_vas_get_active();
	int rc;

	if (!vas || !uk_vas_is_owned(vas))
		return -ENOTSUP;
	rc = uk_vma_unmap_at_exact(vas, (__vaddr_t)addr,
				   &uk_vma_shared_anon_attachment_ops);
	return rc == -ENOENT ? -EINVAL : rc;
}

struct shmid_ds;

UK_SYSCALL_R_DEFINE(int, shmctl, int, id, int, cmd, struct shmid_ds *, buf)
{
	struct uk_vas *vas = uk_vas_get_active();
	struct shm_segment *s;
	struct shm_stat stat;
	__vaddr_t user = (__vaddr_t)buf;
	unsigned long irqf;
	bool free_segment = false;
	int rc = 0;

	if (!vas || !uk_vas_is_owned(vas))
		return -ENOTSUP;
	cmd &= ~IPC_64;
	if (cmd != IPC_STAT && cmd != IPC_RMID)
		return -EINVAL;

	uk_spin_lock_irqsave(&shm_lock, irqf);
	s = shm_find_id(id);
	if (!s) {
		rc = -EINVAL;
	} else if (cmd == IPC_RMID) {
		s->stat.perm.mode |= SHM_DEST;
		s->stat.perm.key = IPC_PRIVATE;
		free_segment = shm_unlink_if_unused(s);
	} else if (!shm_allowed(s, SHM_READ)) {
		rc = -EACCES;
	} else {
		stat = s->stat;
	}
	uk_spin_unlock_irqrestore(&shm_lock, irqf);
	if (free_segment)
		shm_free(s);
	if (rc || cmd == IPC_RMID)
		return rc;

	if (user < vas->range_start || user >= vas->range_end ||
	    sizeof(stat) > vas->range_end - user)
		return -EFAULT;
	return uk_vas_copy_to(vas, user, &stat, sizeof(stat)) ? -EFAULT : 0;
}
