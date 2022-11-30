/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Yuri Volchkov <yuri.volchkov@neclab.eu>
 *
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <uk/fdtab/fd.h>

#include <string.h>
#include <uk/essentials.h>
#include <uk/bitmap.h>
#include <uk/assert.h>
#include <uk/plat/lcpu.h>
#include <errno.h>
#include <uk/fdtab/eventpoll.h>
#include <uk/init.h>

struct fdtab_table {
	unsigned long bitmap[UK_BITS_TO_LONGS(FDTABLE_MAX_FILES)];
	struct fdtab_file *fds[FDTABLE_MAX_FILES];
};

static __uk_tls struct fdtab_table *current_tab;

struct fdtab_table *fdtab_get_active(void)
{
	return current_tab;
}

void fdtab_set_active(struct fdtab_table *tab)
{
	UK_ASSERT(tab);

	current_tab = tab;
}

struct fdtab_table *fdtab_alloc(struct uk_alloc *a)
{
	return uk_zalloc(a, sizeof(struct fdtab_table));
}

struct fdtab_table *fdtab_clone(struct uk_alloc *a, struct fdtab_table *tab)
{
	unsigned long fd;
	struct fdtab_table *clone;

	clone = uk_malloc(a, sizeof(*tab));
	if (clone == NULL)
		return NULL;

	/* Copy all data to clone */
	memcpy(clone->bitmap, tab->bitmap, sizeof(clone->bitmap));
	memcpy(clone->fds, tab->fds, sizeof(clone->fds));

	/* Increase refcount for all file descriptors */
	uk_for_each_set_bit(fd, clone->bitmap, FDTABLE_MAX_FILES)
		fdtab_fhold(tab->fds[fd]);

	return clone;
}

int fdtab_clear(struct fdtab_table *tab)
{
	unsigned long fd;
	unsigned long flags;
	int ret = 0, err;

	flags = ukplat_lcpu_save_irqf();

	uk_for_each_set_bit(fd, tab->bitmap, FDTABLE_MAX_FILES) {
		err = fdtab_put_fd(tab, (int)fd);
		if (err && !ret)
			ret = err;
	}

	ukplat_lcpu_restore_irqf(flags);
	return ret;
}

int fdtab_alloc_fd(struct fdtab_table *tab)
{
	unsigned long flags;
	int ret;

	UK_ASSERT(tab);

	flags = ukplat_lcpu_save_irqf();
	ret = uk_find_next_zero_bit(tab->bitmap, FDTABLE_MAX_FILES, 0);

	if (ret == FDTABLE_MAX_FILES) {
		ret = -ENFILE;
		goto exit;
	}

	uk_bitmap_set(tab->bitmap, ret, 1);

exit:
	ukplat_lcpu_restore_irqf(flags);
	return ret;
}

int fdtab_reserve_fd(struct fdtab_table *tab, int fd)
{
	unsigned long flags;
	int ret = 0;

	flags = ukplat_lcpu_save_irqf();
	if (uk_test_bit(fd, tab->bitmap)) {
		ret = -EBUSY;
		goto exit;
	}

	uk_bitmap_set(tab->bitmap, fd, 1);

exit:
	ukplat_lcpu_restore_irqf(flags);
	return ret;
}

int fdtab_put_fd(struct fdtab_table *tab, int fd)
{
	struct fdtab_file *fp;
	unsigned long flags;

	UK_ASSERT(fd < (int) FDTABLE_MAX_FILES);

	/* FIXME Currently it is not allowed to free std(in|out|err):
	 * if (fd <= 2) return -EBUSY;
	 *
	 * However, returning -EBUSY in this case breaks dup2 with stdin, out,
	 * err. Ignoring this should be fine as long as those are not fdtab_fdrop-ed
	 * twice, in which case the static fp would be freed, and here be
	 * dragons.
	 */

	flags = ukplat_lcpu_save_irqf();
	uk_bitmap_clear(tab->bitmap, fd, 1);
	fp = tab->fds[fd];
	tab->fds[fd] = NULL;
	ukplat_lcpu_restore_irqf(flags);

	/*
	 * Since we can alloc a fd without assigning a
	 * vfsfile we must protect against NULL ptr
	 */
	if (fp)
		fdtab_fdrop(fp);

	return 0;
}

int fdtab_install_fd(struct fdtab_table *tab, int fd, struct fdtab_file *file)
{
	unsigned long flags;
	struct fdtab_file *orig;

	if ((fd >= (int) FDTABLE_MAX_FILES) || (!file))
		return -EBADF;

	fdtab_fhold(file);

	file->fd = fd;

	flags = ukplat_lcpu_save_irqf();
	orig = tab->fds[fd];
	tab->fds[fd] = file;
	ukplat_lcpu_restore_irqf(flags);

	fdtab_fdrop(file);

	if (orig)
		fdtab_fdrop(orig);

	return 0;
}

struct fdtab_file *fdtab_get_file(struct fdtab_table *tab, int fd)
{
	unsigned long flags;
	struct fdtab_file *ret = NULL;

	UK_ASSERT(fd < (int) FDTABLE_MAX_FILES);

	flags = ukplat_lcpu_save_irqf();
	if (!uk_test_bit(fd, tab->bitmap))
		goto exit;
	ret = tab->fds[fd];
	fdtab_fhold(ret);

exit:
	ukplat_lcpu_restore_irqf(flags);
	return ret;
}

void fdtab_put_file(struct fdtab_file *file)
{
	fdtab_fdrop(file);
}

int fdtab_fget(int fd, struct fdtab_file **out_fp)
{
	int ret = 0;
	struct fdtab_table *tab = fdtab_get_active();
	struct fdtab_file *fp = fdtab_get_file(tab, fd);

	if (!fp)
		ret = -EBADF;
	else
		*out_fp = fp;

	return ret;
}

int fdtab_fdalloc(struct fdtab_file *fp, int *newfd)
{
	struct fdtab_table *tab = fdtab_get_active();
	int fd, ret = 0;

	fdtab_fhold(fp);

	fd = fdtab_alloc_fd(tab);
	if (fd < 0) {
		ret = fd;
		goto exit;
	}

	ret = fdtab_install_fd(tab, fd, fp);
	if (ret)
		fdtab_fdrop(fp);
	else
		*newfd = fd;

exit:
	return ret;
}

int fdtab_fdrop(struct fdtab_file *fp)
{
	int prev;

	UK_ASSERT(fp);
	UK_ASSERT(fp->f_count > 0);

	prev = ukarch_dec(&fp->f_count);

	if (prev == 0)
		UK_CRASH("Unbalanced fdtab_fhold/fdtab_fdrop");

	if (prev == 1) {
		eventpoll_notify_close(fp);
		FDOP_FREE(fp);

		return 1;
	}

	return 0;
}

void fdtab_fhold(struct fdtab_file *fp)
{
	ukarch_inc(&fp->f_count);
}

void fdtab_file_init(struct fdtab_file *fp)
{
	memset(fp, 0, sizeof(*fp));
	fp->f_count = 1;
	uk_mutex_init(&fp->f_lock);
	UK_INIT_LIST_HEAD(&fp->f_ep);
}

/* The initial fd table */
static struct fdtab_table init_tab;

static int fdtable_init(void)
{
	fdtab_set_active(&init_tab);
	return 0;
}

#define POSIX_FDTAB_FAMILY_INIT_CLASS UK_INIT_CLASS_LIB
#define POSIX_FDTAB_FAMILY_INIT_PRIO UK_INIT_CLASS_EARLY

uk_initcall_class_prio(fdtable_init,
		       POSIX_FDTAB_FAMILY_INIT_CLASS,
		       POSIX_FDTAB_FAMILY_INIT_PRIO);
