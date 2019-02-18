/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (C) 2013 Cloudius Systems, Ltd.
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation.
 *
 * All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <osv/file.h>
#include <osv/poll.h>
#include <fs/vfs/vfs.h>
#include <osv/vfs_file.hh>
#include <osv/mmu.hh>
#include <osv/pagecache.hh>

vfs_file::vfs_file(unsigned flags)
	: file(flags, DTYPE_VNODE)
{
}

int vfs_file::close()
{
	auto fp = this;
	struct vnode *vp = fp->f_dentry->d_vnode;
	int error;

	vn_lock(vp);
	error = VOP_CLOSE(vp, fp);
	vn_unlock(vp);

	if (error)
		return error;

	fp->f_dentry.reset();
	return 0;
}

int vfs_file::read(struct uio *uio, int flags)
{
	auto fp = this;
	struct vnode *vp = fp->f_dentry->d_vnode;
	int error;
	size_t count;
	ssize_t bytes;

	bytes = uio->uio_resid;

	vn_lock(vp);
	if ((flags & FOF_OFFSET) == 0)
		uio->uio_offset = fp->f_offset;

	error = VOP_READ(vp, fp, uio, 0);
	if (!error) {
		count = bytes - uio->uio_resid;
		if ((flags & FOF_OFFSET) == 0)
			fp->f_offset += count;
	}
	vn_unlock(vp);

	return error;
}


int vfs_file::write(struct uio *uio, int flags)
{
	auto fp = this;
	struct vnode *vp = fp->f_dentry->d_vnode;
	int ioflags = 0;
	int error;
	size_t count;
	ssize_t bytes;

	bytes = uio->uio_resid;

	vn_lock(vp);

	if (fp->f_flags & O_APPEND)
		ioflags |= IO_APPEND;
	if (fp->f_flags & (O_DSYNC|O_SYNC))
		ioflags |= IO_SYNC;

	if ((flags & FOF_OFFSET) == 0)
	        uio->uio_offset = fp->f_offset;

	error = VOP_WRITE(vp, uio, ioflags);
	if (!error) {
		count = bytes - uio->uio_resid;
		if ((flags & FOF_OFFSET) == 0)
			fp->f_offset += count;
	}

	vn_unlock(vp);
	return error;
}

int vfs_file::ioctl(u_long com, void *data)
{
	auto fp = this;
	struct vnode *vp = fp->f_dentry->d_vnode;
	int error;

	vn_lock(vp);
	error = VOP_IOCTL(vp, fp, com, data);
	vn_unlock(vp);

	return error;
}

int vfs_file::stat(struct stat *st)
{
	auto fp = this;
	struct vnode *vp = fp->f_dentry->d_vnode;
	int error;

	vn_lock(vp);
	error = vn_stat(vp, st);
	vn_unlock(vp);

	return error;
}

int vfs_file::poll(int events)
{
	return poll_no_poll(events);
}

int vfs_file::truncate(off_t len)
{
	// somehow this is handled outside file ops
	abort();
}

int vfs_file::chmod(mode_t mode)
{
	// somehow this is handled outside file ops
	abort();
}

bool vfs_file::map_page(uintptr_t off, mmu::hw_ptep<0> ptep, mmu::pt_element<0> pte, bool write, bool shared)
{
	return pagecache::get(this, off, ptep, pte, write, shared);
}

bool vfs_file::put_page(void *addr, uintptr_t off, mmu::hw_ptep<0> ptep)
{
	return pagecache::release(this, addr, off, ptep);
}

void vfs_file::sync(off_t start, off_t end)
{
	pagecache::sync(this, start, end);
}

// Locking: VOP_CACHE will call into the filesystem, and that can trigger an
// eviction that will hold the mmu-side lock that protects the mappings
// Always follow that order. We however can't just get rid of the mmu-side lock,
// because not all invalidations will be synchronous.
int vfs_file::get_arcbuf(void* key, off_t offset)
{
	struct vnode *vp = f_dentry->d_vnode;

	iovec io[1];

	io[0].iov_base = key;
	uio data;
	data.uio_iov = io;
	data.uio_iovcnt = 1;
	data.uio_offset = offset;
	data.uio_resid = mmu::page_size;
	data.uio_rw = UIO_READ;

	vn_lock(vp);
	assert(VOP_CACHE(vp, this, &data) == 0);
	vn_unlock(vp);

	return (data.uio_resid != 0) ? -1 : 0;
}

std::unique_ptr<mmu::file_vma> vfs_file::mmap(addr_range range, unsigned flags, unsigned perm, off_t offset)
{
	auto fp = this;
	struct vnode *vp = fp->f_dentry->d_vnode;
	if (!vp->v_op->vop_cache || (vp->v_size < (off_t)mmu::page_size)) {
		return mmu::default_file_mmap(this, range, flags, perm, offset);
	}
	return mmu::map_file_mmap(this, range, flags, perm, offset);
}
