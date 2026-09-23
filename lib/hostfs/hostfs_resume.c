/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors. */

/*
 * hostfs after a snapshot restore.
 *
 * The mount table is guest memory, so a restored guest still has the
 * hostfs mounts of the snapshot, while the host that restored it serves
 * whatever mounts its embedder gave it: the same ones, more, fewer, or
 * others.  Every hostfs host call names its mount by index, so a table
 * out of step with the host would read one directory through another's
 * index, or fail.  A snapshot taken without mounts, the usual warm image,
 * could not serve a mount at all.
 *
 * On resume the kernel asks the host for its list (GetMounts: the fstab
 * entries the boot cmdline would carry) and makes the table match it.  A
 * mount the host no longer serves is unmounted; one it now serves is
 * mounted, its mount point created first as the boot's mkmp would; one
 * whose index or read-only flag changed is unmounted and mounted again,
 * since every node under it carries the index.  A host without GetMounts
 * serves the snapshot's mounts, or none, and the table is left alone.
 *
 * This runs at a boundary, with every guest thread parked, on the resume
 * entry: nothing is using the mounts meanwhile.  A file left open across
 * the snapshot on a mount that then vanished keeps its dentries, so the
 * unmount fails with EBUSY and that mount stays, dead: the host serves
 * its new mounts under the indices now, so the stale mount's nodes fail
 * with ESTALE rather than reach another mount's directory (see
 * hostfs_stale()), and the next restore that finds it free drops it.  A
 * mount under another keeps the outer one busy the same way, so the
 * unmount pass repeats until nothing more comes off.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <uk/essentials.h>
#include <uk/list.h>
#include <uk/print.h>
#include <vfscore/mount.h>

#include <hyperlight-x86/hcall.h>

#include "hostfs.h"

/* The host's list, at most: a quarter of what one host call carries. */
#define HOSTFS_RESUME_LIST_MAX	16384
/* Mounts the host may serve; the list above bounds it well before this. */
#define HOSTFS_RESUME_MOUNTS_MAX	32
/* A mount point, as long as a node path. */
#define HOSTFS_RESUME_PATH_MAX	1024

/* Every hostfs mount, in mount order. */
static UK_LIST_HEAD(hostfs_mnts);

struct hostfs_mnt *hostfs_mnt_add(struct mount *mp, int idx)
{
	struct hostfs_mnt *m;

	m = malloc(sizeof(*m));
	if (unlikely(!m)) {
		/* The mount works; it just cannot be reconciled on a
		 * restore.
		 */
		uk_pr_err("hostfs: no memory to register mount %s\n",
			  mp->m_path);
		return NULL;
	}
	m->mp = mp;
	m->idx = idx;
	m->stale_rc = 0;
	m->dead = 0;
	uk_list_add_tail(&m->list, &hostfs_mnts);
	return m;
}

void hostfs_mnt_del(struct mount *mp)
{
	struct hostfs_mnt *m;

	uk_list_for_each_entry(m, &hostfs_mnts, list) {
		if (m->mp != mp)
			continue;
		uk_list_del(&m->list);
		free(m);
		return;
	}
}

/* One entry of the host's list, as far as hostfs cares. */
struct hostfs_want {
	const char *sdev;	/* the host's index text, for mount() */
	const char *path;
	int idx;
	unsigned long flags;
	int satisfied;		/* the table has it already */
};

/*
 * Split the host's list in place.  Entries are separated by spaces and
 * fields by colons, the fstab volume syntax: sdev:path:drv:flags[:opts:
 * ukopts].  Entries of other drivers are not hostfs's to reconcile and
 * are skipped, as is anything malformed, which is reported.  Returns how
 * many entries were kept.
 */
static int hostfs_parse_mounts(char *list, struct hostfs_want *want, int max)
{
	char *entry = list;
	int n = 0;

	while (entry && *entry) {
		char *next = strchr(entry, ' ');
		char *field[4] = { NULL, NULL, NULL, NULL };
		char *pos = entry;
		char *endp;
		long idx;
		int i;

		if (next)
			*next++ = '\0';

		for (i = 0; i < 4 && pos; i++) {
			field[i] = pos;
			pos = strchr(pos, ':');
			if (pos)
				*pos++ = '\0';
		}
		if (i < 4 || strcmp(field[2], "hostfs") != 0)
			goto skip;
		if (field[1][0] != '/') {
			uk_pr_err("hostfs: GetMounts: mount point \"%s\" is "
				  "not absolute\n", field[1]);
			goto skip;
		}
		idx = strtol(field[0], &endp, 10);
		if (endp == field[0] || *endp != '\0' || idx < 0) {
			uk_pr_err("hostfs: GetMounts: \"%s\" is not a mount "
				  "index\n", field[0]);
			goto skip;
		}
		if (n == max) {
			uk_pr_err("hostfs: GetMounts: more than %d mounts; "
				  "ignoring the rest\n", max);
			break;
		}
		want[n].sdev = field[0];
		want[n].path = field[1];
		want[n].idx = (int)idx;
		want[n].flags = strtoul(field[3], NULL, 0);
		want[n].satisfied = 0;
		n++;
skip:
		entry = next;
	}
	return n;
}

/* The host's entry for the mount point of m, if it still has one. */
static struct hostfs_want *hostfs_want_for(struct hostfs_want *want, int n,
					   const struct hostfs_mnt *m)
{
	for (int i = 0; i < n; i++)
		if (!strcmp(want[i].path, m->mp->m_path))
			return &want[i];
	return NULL;
}

/* mkdir -p for a mount point, as the boot's mkmp does. */
static int hostfs_mkmp(const char *path)
{
	char buf[HOSTFS_RESUME_PATH_MAX];
	char *pos;

	if (strlen(path) >= sizeof(buf))
		return -ENAMETOOLONG;
	strcpy(buf, path);

	for (pos = strchr(buf + 1, '/'); pos; pos = strchr(pos + 1, '/')) {
		*pos = '\0';
		if (mkdir(buf, 0755) < 0 && errno != EEXIST)
			return -errno;
		*pos = '/';
	}
	if (mkdir(buf, 0755) < 0 && errno != EEXIST)
		return -errno;
	return 0;
}

void hostfs_resume(void)
{
	/* Static: the resume entry runs on a thread stack, and one call at
	 * a time.
	 */
	static char list[HOSTFS_RESUME_LIST_MAX];
	struct hostfs_want want[HOSTFS_RESUME_MOUNTS_MAX];
	struct hostfs_mnt *m, *tmp;
	__sz len = 0;
	int n, i, rc;

	/* The kernel and the host ship together, so a host without
	 * GetMounts is an external-kernel setup; say so rather than serve
	 * the snapshot's mounts in silence.
	 */
	if (hl_hcall_string("GetMounts", NULL, 0, list, sizeof(list),
			    &len) < 0) {
		uk_pr_warn("hostfs: GetMounts failed; keeping the snapshot's "
			   "mounts\n");
		return;
	}
	n = hostfs_parse_mounts(list, want, ARRAY_SIZE(want));

	/* Drop what the host no longer serves, and what it serves under
	 * another index or flags: the pass below mounts those afresh.  A
	 * mount under another keeps the outer one busy, so go newest first,
	 * and pass again while something came off.
	 */
	do {
		int progress = 0;

		uk_list_for_each_entry_safe_reverse(m, tmp, &hostfs_mnts,
						    list) {
			struct hostfs_want *w = hostfs_want_for(want, n, m);

			if (w && !m->dead && w->idx == m->idx &&
			    (w->flags & MNT_RDONLY) ==
			    (unsigned long)(m->mp->m_flags & MNT_RDONLY)) {
				w->satisfied = 1;
				continue;
			}
			/* hostfs_unmount() takes m off the list.  vfscore
			 * hands some failures back as a positive errno rather
			 * than through errno.
			 */
			rc = umount2(m->mp->m_path, 0);
			if (rc == 0) {
				progress = 1;
				continue;
			}
			m->stale_rc = rc < 0 ? errno : rc;
		}
		if (!progress)
			break;
	} while (1);

	/* What would not come off stays, dead: the host no longer serves
	 * it, so its nodes fail (see hostfs_stale()); and its path is
	 * taken, so the host's entry for it, if any, waits for a restore
	 * that finds the mount free.
	 */
	uk_list_for_each_entry(m, &hostfs_mnts, list) {
		struct hostfs_want *w = hostfs_want_for(want, n, m);

		if (!m->stale_rc)
			continue;
		uk_pr_err("hostfs: %s stays mounted after restore, %s: %d\n",
			  m->mp->m_path, w ? "changed" : "gone", m->stale_rc);
		m->stale_rc = 0;
		m->dead = 1;
		if (w)
			w->satisfied = 1;
	}

	/* Mount what the host now serves. */
	for (i = 0; i < n; i++) {
		if (want[i].satisfied)
			continue;
		rc = hostfs_mkmp(want[i].path);
		if (rc < 0) {
			uk_pr_err("hostfs: cannot create mount point %s after "
				  "restore: %d\n", want[i].path, -rc);
			continue;
		}
		if (mount(want[i].sdev, want[i].path, "hostfs", want[i].flags,
			  NULL) < 0) {
			uk_pr_err("hostfs: cannot mount %s (index %d) after "
				  "restore: %d\n",
				  want[i].path, want[i].idx, errno);
			continue;
		}
		uk_pr_info("hostfs: mounted %s (index %d) after restore\n",
			   want[i].path, want[i].idx);
	}
}
