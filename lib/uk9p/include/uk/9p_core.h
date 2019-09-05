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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
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
