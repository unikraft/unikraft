/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Banu <cristb@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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

#ifndef __UK_9P_CORE__
#define __UK_9P_CORE__

#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <uk/assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Documentation of the protocol may be found here:
 * https://9fans.github.io/plan9port/
 */

/**
 * 9P request types.
 *
 * Source: https://github.com/9fans/plan9port/blob/master/include/fcall.h
 */
enum uk_9p_type {
	UK_9P_TLERROR = 6,
	UK_9P_RLERROR,
	UK_9P_TSTATFS = 8,
	UK_9P_RSTATFS,
	UK_9P_TLOPEN = 12,
	UK_9P_RLOPEN,
	UK_9P_TLCREATE = 14,
	UK_9P_RLCREATE,
	UK_9P_TSYMLINK = 16,
	UK_9P_RSYMLINK,
	UK_9P_TMKNOD = 18,
	UK_9P_RMKNOD,
	UK_9P_TRENAME = 20,
	UK_9P_RRENAME,
	UK_9P_TREADLINK = 22,
	UK_9P_RREADLINK,
	UK_9P_TGETATTR = 24,
	UK_9P_RGETATTR,
	UK_9P_TSETATTR = 26,
	UK_9P_RSETATTR,
	UK_9P_TXATTRWALK = 30,
	UK_9P_RXATTRWALK,
	UK_9P_TXATTRCREATE = 32,
	UK_9P_RXATTRCREATE,
	UK_9P_TREADDIR = 40,
	UK_9P_RREADDIR,
	UK_9P_TFSYNC = 50,
	UK_9P_RFSYNC,
	UK_9P_TLOCK = 52,
	UK_9P_RLOCK,
	UK_9P_TGETLOCK = 54,
	UK_9P_RGETLOCK,
	UK_9P_TLINK = 70,
	UK_9P_RLINK,
	UK_9P_TMKDIR = 72,
	UK_9P_RMKDIR,
	UK_9P_TRENAMEAT = 74,
	UK_9P_RRENAMEAT,
	UK_9P_TUNLINKAT = 76,
	UK_9P_RUNLINKAT,
	UK_9P_TVERSION          = 100,
	UK_9P_RVERSION,
	UK_9P_TAUTH             = 102,
	UK_9P_RAUTH,
	UK_9P_TATTACH           = 104,
	UK_9P_RATTACH,
	UK_9P_TERROR            = 106,
	UK_9P_RERROR,
	UK_9P_TFLUSH            = 108,
	UK_9P_RFLUSH,
	UK_9P_TWALK             = 110,
	UK_9P_RWALK,
	UK_9P_TOPEN             = 112,
	UK_9P_ROPEN,
	UK_9P_TCREATE           = 114,
	UK_9P_RCREATE,
	UK_9P_TREAD             = 116,
	UK_9P_RREAD,
	UK_9P_TWRITE            = 118,
	UK_9P_RWRITE,
	UK_9P_TCLUNK            = 120,
	UK_9P_RCLUNK,
	UK_9P_TREMOVE           = 122,
	UK_9P_RREMOVE,
	UK_9P_TSTAT             = 124,
	UK_9P_RSTAT,
	UK_9P_TWSTAT            = 126,
	UK_9P_RWSTAT,
};

/**
 * 9P values of a qid.type.
 *
 * Sources: https://9fans.github.io/plan9port/man/man9/intro.html,
 * http://ericvh.github.io/9p-rfc/rfc9p2000.u.html.
 */
#define UK_9P_QTDIR               0x80
#define UK_9P_QTAPPEND            0x40
#define UK_9P_QTEXCL              0x20
#define UK_9P_QTMOUNT             0x10
#define UK_9P_QTAUTH              0x08
#define UK_9P_QTTMP               0x04
#define UK_9P_QTSYMLINK           0x02
#define UK_9P_QTLINK              0x01
#define UK_9P_QTFILE              0x00

/**
 * 9P permission bits.
 *
 * Sources: https://9fans.github.io/plan9port/man/man9/intro.html,
 * http://ericvh.github.io/9p-rfc/rfc9p2000.u.html.
 */
#define UK_9P_DMDIR               0x80000000
#define UK_9P_DMAPPEND            0x40000000
#define UK_9P_DMEXCL              0x20000000
#define UK_9P_DMMOUNT             0x10000000
#define UK_9P_DMAUTH              0x08000000
#define UK_9P_DMTMP               0x04000000
#define UK_9P_DMSYMLINK           0x02000000
#define UK_9P_DMLINK              0x01000000
#define UK_9P_DMDEVICE            0x00800000
#define UK_9P_DMNAMEDPIPE         0x00200000
#define UK_9P_DMSOCKET            0x00100000
#define UK_9P_DMSETUID            0x00080000
#define UK_9P_DMSETGID            0x00040000
#define UK_9P_DMSETVTX            0x00010000

/**
 * 9P open mode bits.
 *
 * Source: https://9fans.github.io/plan9port/man/man9/open.html.
 */
#define UK_9P_OREAD               0x00
#define UK_9P_OWRITE              0x01
#define UK_9P_ORDWR               0x02
#define UK_9P_OEXEC               0x03
#define UK_9P_OTRUNC              0x10
#define UK_9P_OREXEC              0x20
#define UK_9P_ORCLOSE             0x40
#define UK_9P_OAPPEND             0x80
#define UK_9P_OEXCL               0x1000

#define UK_9P_GETATTR_MODE        0x00000001
#define UK_9P_GETATTR_NLINK       0x00000002
#define UK_9P_GETATTR_UID         0x00000004
#define UK_9P_GETATTR_GID         0x00000008
#define UK_9P_GETATTR_RDEV        0x00000010
#define UK_9P_GETATTR_ATIME       0x00000020
#define UK_9P_GETATTR_MTIME       0x00000040
#define UK_9P_GETATTR_CTIME       0x00000080
#define UK_9P_GETATTR_INO         0x00000100
#define UK_9P_GETATTR_SIZE        0x00000200
#define UK_9P_GETATTR_BLOCKS      0x00000400

#define UK_9P_GETATTR_BTIME        0x00000800
#define UK_9P_GETATTR_GEN          0x00001000
#define UK_9P_GETATTR_DATA_VERSION 0x00002000

#define UK_9P_GETATTR_BASIC       0x000007ff /* Mask for fields up to BLOCKS */
#define UK_9P_GETATTR_ALL         0x00003fff /* Mask for All fields above */

#define UK_9P_SETATTR_MODE        0x00000001UL
#define UK_9P_SETATTR_UID         0x00000002UL
#define UK_9P_SETATTR_GID         0x00000004UL
#define UK_9P_SETATTR_SIZE        0x00000008UL
#define UK_9P_SETATTR_ATIME       0x00000010UL
#define UK_9P_SETATTR_MTIME       0x00000020UL
#define UK_9P_SETATTR_CTIME       0x00000040UL
#define UK_9P_SETATTR_ATIME_SET   0x00000080UL
#define UK_9P_SETATTR_MTIME_SET   0x00000100UL

/**
 * 9p2000.L open flags
 *
 * Source: https://lxr.missinglinkelectronics.com/qemu/hw/9pfs/9p.h#L368
 */
#define UK_9P_DOTL_RDONLY        00000000
#define UK_9P_DOTL_WRONLY        00000001
#define UK_9P_DOTL_RDWR          00000002
#define UK_9P_DOTL_NOACCESS      00000003
#define UK_9P_DOTL_CREATE        00000100
#define UK_9P_DOTL_EXCL          00000200
#define UK_9P_DOTL_NOCTTY        00000400
#define UK_9P_DOTL_TRUNC         00001000
#define UK_9P_DOTL_APPEND        00002000
#define UK_9P_DOTL_NONBLOCK      00004000
#define UK_9P_DOTL_DSYNC         00010000
#define UK_9P_DOTL_FASYNC        00020000
#define UK_9P_DOTL_DIRECT        00040000
#define UK_9P_DOTL_LARGEFILE     00100000
#define UK_9P_DOTL_DIRECTORY     00200000
#define UK_9P_DOTL_NOFOLLOW      00400000
#define UK_9P_DOTL_NOATIME       01000000
#define UK_9P_DOTL_CLOEXEC       02000000
#define UK_9P_DOTL_SYNC          04000000

/**
 * 9P qid.
 *
 * Source: https://9fans.github.io/plan9port/man/man9/intro.html.
 */
struct uk_9p_qid {
	uint8_t                 type;
	uint32_t                version;
	uint64_t                path;
};

/**
 * 9P string.
 *
 * Source: https://9fans.github.io/plan9port/man/man9/intro.html.
 */
struct uk_9p_str {
	uint16_t                size;
	char                    *data;
};

/**
 * Check if a 9P string is equal to a given null-terminated string.
 *
 * @param s
 *   9P string.
 * @param p
 *   Null-terminated string.
 * @return
 *   1 if equal, 0 otherwise.
 */
static inline int uk_9p_str_equal(const struct uk_9p_str *s, const char *p)
{
	return strlen(p) == s->size && !strncmp(s->data, p, s->size);
}

/**
 * Initialize a 9P string from a given null-terminated string.
 *
 * @param s
 *   9P string.
 * @param p
 *   Null-terminated string.
 */
static inline void uk_9p_str_init(struct uk_9p_str *s, const char *p)
{
	if (!p) {
		s->size = 0;
		return;
	}

	s->size = strlen(p);
	s->data = (char *)p;
}

/**
 * 9P stat structure.
 */
struct uk_9p_stat {
	uint16_t                size;
	uint16_t                type;
	uint32_t                dev;
	struct uk_9p_qid        qid;
	uint32_t                mode;
	uint32_t                atime;
	uint32_t                mtime;
	uint64_t                length;
	struct uk_9p_str        name;
	struct uk_9p_str        uid;
	struct uk_9p_str        gid;
	struct uk_9p_str        muid;
	struct uk_9p_str        extension;
	uint32_t                n_uid;
	uint32_t                n_gid;
	uint32_t                n_muid;
};

/**
 * 9P2000.L stat structure.
 */
struct uk_9p_attr {
	uint64_t				valid;
	struct uk_9p_qid		qid;
	uint32_t				mode;
	uint32_t				uid;
	uint32_t				gid;
	uint64_t				nlink;
	uint64_t				rdev;
	uint64_t				size;
	uint64_t				blksize;
	uint64_t				blocks;
	uint64_t				atime_sec;
	uint64_t				atime_nsec;
	uint64_t				mtime_sec;
	uint64_t				mtime_nsec;
	uint64_t				ctime_sec;
	uint64_t				ctime_nsec;
	uint64_t				btime_sec;
	uint64_t				btime_nsec;
	uint64_t				gen;
	uint64_t				data_version;
};

/*
 * TODO: The wire format is always little-endian. Add little-endian types and
 * cpu_to_le*() data to the required format.
 */

/**
 * Initialize a 9P stat structure that won't modify any fields if sent with a
 * WSTAT message, by setting all integer fields to ~0 and all string fields to
 * empty strings.
 *
 * @param stat
 *   The 9P stat structure.
 */
static inline void uk_9p_stat_init(struct uk_9p_stat *stat)
{
	UK_ASSERT(stat);
	memset(stat, ~0, sizeof(struct uk_9p_stat));
	stat->name.size = 0;
	stat->uid.size = 0;
	stat->gid.size = 0;
	stat->muid.size = 0;
	stat->extension.size = 0;
}

/**
 * No tag, used by the Tversion/Rversion pair of request/reply messages.
 */
#define UK_9P_NOTAG                    UINT16_MAX

/**
 * Maximum available tag for use.
 * UK_9P_NOTAG (~0) is reserved by the 9P RFC for representing no tag.
 */
#define UK_9P_MAXTAG                   (UINT16_MAX - 1)

/**
 * Number of possible tags, including NOTAG.
 */
#define UK_9P_NUMTAGS                  ((uint32_t)(UINT16_MAX) + 1)

/**
 * No fid, used to mark a fid field as unused.
 */
#define UK_9P_NOFID                    UINT32_MAX

/**
 * No n_uname in TATTACH requests, used to mark the field as unused.
 */
#define UK_9P_NONUNAME                 UINT32_MAX

#ifdef __cplusplus
}
#endif

#endif /* __UK_9P_CORE__ */
