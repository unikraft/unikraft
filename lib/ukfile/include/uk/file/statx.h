/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKFILE_STATX_H__
#define __UKFILE_STATX_H__

#include <stdint.h>

/*
 * Linux-compatible `statx` structure for use in syscalls, along with
 * definitions of bit flags.
 *
 * Layout taken from statx(2) man page with paddings from musl v1.2.3.
 * Flag values taken from Linux headers v6.5.6 (include/uapi/linux/stat.h).
 */

struct uk_statx_timestamp {
	int64_t tv_sec;
	uint32_t tv_nsec;
	int32_t _reserved;
};

struct uk_statx {
	uint32_t stx_mask;
	uint32_t stx_blksize;
	uint64_t stx_attributes;
	uint32_t stx_nlink;
	uint32_t stx_uid;
	uint32_t stx_gid;
	uint16_t stx_mode;
	uint16_t _mode_reserved;
	uint64_t stx_ino;
	uint64_t stx_size;
	uint64_t stx_blocks;
	uint64_t stx_attributes_mask;
	struct uk_statx_timestamp stx_atime, stx_btime, stx_ctime, stx_mtime;
	uint32_t stx_rdev_major;
	uint32_t stx_rdev_minor;
	uint32_t stx_dev_major;
	uint32_t stx_dev_minor;
	uint64_t stx_mnt_id;
	uint32_t stx_dio_mem_align;
	uint32_t stx_dio_offset_align;
	uint64_t _spare[12];
};

/* Bits used in stx_mask */
#define UK_STATX_TYPE        0x00000001U /* File type in stx_mode */
#define UK_STATX_MODE        0x00000002U /* File mode (perms) in stx_mode */
#define UK_STATX_NLINK       0x00000004U /* stx_nlink */
#define UK_STATX_UID         0x00000008U /* stx_uid */
#define UK_STATX_GID         0x00000010U /* stx_gid */
#define UK_STATX_ATIME       0x00000020U /* stx_atime */
#define UK_STATX_MTIME       0x00000040U /* stx_mtime */
#define UK_STATX_CTIME       0x00000080U /* stx_ctime */
#define UK_STATX_BTIME       0x00000800U /* stx_btime */
#define UK_STATX_INO         0x00000100U /* stx_ino */
#define UK_STATX_SIZE        0x00000200U /* stx_size */
#define UK_STATX_BLOCKS      0x00000400U /* stx_blocks */
#define UK_STATX_MNT_ID      0x00001000U /* stx_mnt_id */
#define UK_STATX_DIOALIGN    0x00002000U /* stx_dio_*_align */

#define UK_STATX_BASIC_STATS 0x000007ffU /* Fields common with `struct stat` */
#define UK_STATX__RESERVED   0x80000000U /* Reserved bit */

/* Bits used in stx_attributes and stx_attributes_mask */
#define UK_STATX_ATTR_COMPRESSED 0x00000004 /* File is compressed by the fs */
#define UK_STATX_ATTR_IMMUTABLE  0x00000010 /* File is marked immutable */
#define UK_STATX_ATTR_APPEND     0x00000020 /* File is append-only */
#define UK_STATX_ATTR_NODUMP     0x00000040 /* File is not to be dumped */
#define UK_STATX_ATTR_ENCRYPTED  0x00000800 /* Requires key to decrypt in fs */
#define UK_STATX_ATTR_AUTOMOUNT  0x00001000 /* Dir: Automount trigger */
#define UK_STATX_ATTR_MOUNT_ROOT 0x00002000 /* Root of a mount */
#define UK_STATX_ATTR_VERITY     0x00100000 /* Verity protected file */
#define UK_STATX_ATTR_DAX        0x00200000 /* File is currently in DAX state */

#endif /* __UKFILE_STATX_H__ */
