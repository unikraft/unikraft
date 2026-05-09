/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __HOSTFS_H__
#define __HOSTFS_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <uk/arch/types.h>

/* Max path length (mount-relative). */
#define HOSTFS_MAX_PATH 1024

/* Max single read/write chunk going over __dispatch. Bounded below the
 * hyperlight_hcall() encode buffer (64 KiB). Larger transfers are split
 * across multiple RPCs by the read/write vnops.
 */
#define HOSTFS_CHUNK 32768

/* Per-vnode data — just the mount-relative path. */
struct hostfs_node {
	char path[HOSTFS_MAX_PATH];
};

/*
 * RPC layer — builds JSON requests for the host's FsSandbox handlers
 * and decodes responses. Declared in hostfs_rpc.c.
 */

struct hostfs_stat {
	uint64_t size;
	int is_dir;
	int is_file;
};

/* Result codes: 0 on success, -errno on failure. Errors map to
 * ENOENT/EACCES/etc as best we can infer from the host's error string.
 */
int hostfs_rpc_stat(const char *path, struct hostfs_stat *out);
int hostfs_rpc_read(const char *path, uint64_t offset,
		    void *buf, size_t cap, size_t *out_len, int *eof);
int hostfs_rpc_write(const char *path, uint64_t offset,
		     const void *buf, size_t len, int append);
int hostfs_rpc_mkdir(const char *path);
int hostfs_rpc_unlink(const char *path);
int hostfs_rpc_truncate(const char *path, uint64_t length);

/* Streamed readdir: caller passes an index; on success `name` is
 * filled with the N-th entry's name (null-terminated) and `is_dir`
 * tells the entry type. ENOENT if index is past the end.
 */
int hostfs_rpc_readdir(const char *path, size_t index,
		       char *name, size_t name_cap, int *is_dir);

/*
 * Base64 codec (RFC 4648 standard alphabet, no line breaks).
 * hostfs_b64_encoded_len(n)  = 4 * ceil(n / 3)
 * hostfs_b64_decoded_cap(n)  = (n / 4) * 3   — upper bound
 */
size_t hostfs_b64_encoded_len(size_t n);
size_t hostfs_b64_decoded_cap(size_t n);
size_t hostfs_b64_encode(const void *src, size_t n, char *dst);
/* Returns the number of decoded bytes written to dst, or -1 on invalid input. */
long hostfs_b64_decode(const char *src, size_t n, void *dst, size_t cap);

#endif /* __HOSTFS_H__ */
