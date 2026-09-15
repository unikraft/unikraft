/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors. */

#ifndef __HOSTFS_H__
#define __HOSTFS_H__

#include <uk/arch/types.h>
#include <vfscore/vnode.h>

/*
 * Transfer sizing and buffers, set up by the first mount (hostfs_io_init):
 *
 *   chunk       data bytes per fs_read_bytes / fs_write_bytes -- the
 *               host's preference (GetHostFsChunkSize), never more than
 *               one host call carries (hl_hcall_max_payload);
 *   result_max  what any result can be, i.e. the size of rbuf.
 *
 * Every result carries an i32 status ahead of its payload.  The buffers
 * are heap -- a payload does not fit a thread stack -- and shared: the
 * single vCPU runs one host call at a time, and no hostfs operation
 * yields while it holds them.
 */
#define HOSTFS_STATUS_LEN 4

struct hostfs_io {
	size_t chunk;
	size_t result_max;
	__u8 *rbuf;	/* results: fs_read_bytes, fs_list, fs_readlink */
	__u8 *wbuf;	/* the fs_write_bytes payload, chunk bytes */
};

extern struct hostfs_io hostfs_io;

/* Size the transfers and allocate the buffers; 0 or an errno. */
int hostfs_io_init(void);

/*
 * hostfs stores minimal per-node metadata.  Actual data lives on the
 * host; every operation goes through an hl_hcall_* round-trip.
 *
 * The v_data field of every hostfs vnode points to a struct hostfs_node
 * whose hf_path is the host-relative path (relative to the mount root).
 */
struct hostfs_node {
	char	hf_path[1024];	/* host-relative path */
	int	hf_type;	/* VREG or VDIR */
	int	hf_mount_idx;	/* mount index for host calls */
};

#endif /* __HOSTFS_H__ */
