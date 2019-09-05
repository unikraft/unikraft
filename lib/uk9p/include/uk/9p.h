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
#ifndef __UK_9P__
#define __UK_9P__

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <uk/config.h>
#include <uk/9p_core.h>
#include <uk/9pdev.h>
#include <uk/9preq.h>
#include <uk/9pfid.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Negotiates the version and is the first message in a 9P session.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param requested
 *   Requested version string.
 * @param received
 *   Received version string.
 * @return
 *   - (!ERRPTR): The request. It must be removed only after all accesses to
 *   the received version string are done.
 *   - ERRPTR: The error returned either by the API or by the remote server.
 */
struct uk_9preq *uk_9p_version(struct uk_9pdev *dev,
		const char *requested, struct uk_9p_str *received);

/**
 * Attaches to a filesystem tree exported by the 9P server, returning the
 * fid of the root directory.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param afid
 *   Authentication fid, usually UK_9P_NOFID.
 * @param uname
 *   User name, can be empty string for virtio/xen.
 * @param aname
 *   The file tree to access, can be left empty for virtio/xen.
 * @param n_uname
 *   Numeric uname, part of the 9P2000.u unix extension to the protocol.
 * @return
 *   - (!ERRPTR): The fid of the root directory in the accessed file tree.
 *   - ERRPTR: The error returned either by the API or by the remote server.
 */
struct uk_9pfid *uk_9p_attach(struct uk_9pdev *dev, uint32_t afid,
		const char *uname, const char *aname, uint32_t n_uname);

/**
 * Flushes the given request tag, canceling the corresponding request if
 * the server has not yet replied to it.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param oldtag
 *   Request tag.
 * @return
 *   - 0: Successful.
 *   - (< 0): An error occurred.
 */
int uk_9p_flush(struct uk_9pdev *dev, uint16_t oldtag);

/**
 * Walks the filesystem tree from the given directory fid, attempting to obtain
 * the fid for the child with the given name.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param fid
 *   Directory fid.
 * @param name
 *   File name.
 * @return
 *   - (!ERRPTR): The fid of the child entry.
 *   - ERRPTR: The error returned either by the API or by the remote server.
 */
struct uk_9pfid *uk_9p_walk(struct uk_9pdev *dev, struct uk_9pfid *fid,
		const char *name);

/**
 * Opens the fid with the given mode.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param fid
 *   9P fid.
 * @param mode
 *   9P open mode.
 * @return
 *   - 0: Successful.
 *   - (< 0): An error occurred.
 */
int uk_9p_open(struct uk_9pdev *dev, struct uk_9pfid *fid, uint8_t mode);

/**
 * Creates a new file with the given name in the directory associated with fid,
 * and associates fid with the newly created file, opening it with the given
 * mode.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param fid
 *   9P directory fid.
 * @param name
 *   Name of the created file.
 * @param perm
 *   9P permission bits.
 * @param mode
 *   9P open mode.
 * @param extension
 *   String describing special files, depending on the mode bit.
 * @return
 *   - 0: Successful.
 *   - (< 0): An error occurred.
 */
int uk_9p_create(struct uk_9pdev *dev, struct uk_9pfid *fid,
		const char *name, uint32_t perm, uint8_t mode,
		const char *extension);

/**
 * Removes the file associated with fid.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param fid
 *   9P fid to remove.
 * @return
 *   - 0: Successful.
 *   - (< 0): An error occurred.
 */
int uk_9p_remove(struct uk_9pdev *dev, struct uk_9pfid *fid);

/**
 * Clunks the fid, telling the server to forget its previous association.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param fid
 *   9P fid to clunk.
 * @return
 *   - 0: Successful.
 *   - (< 0): An error occurred.
 */
int uk_9p_clunk(struct uk_9pdev *dev, struct uk_9pfid *fid);

/**
 * Reads count bytes from the fid, starting from the given offset, placing
 * them into the buffer. As in the case of POSIX read(), the number of
 * bytes read may be less than count, which is not an error, but rather
 * signals that the offset is close to EOF or count is too big for the
 * transport of this 9P device. A return value of 0 indicates end of
 * file.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param fid
 *   9P fid to read from.
 * @param offset
 *   Offset at which to start reading.
 * @param count
 *   Maximum number of bytes to read.
 * @param buf
 *   Buffer to read into.
 * @return
 *   - (>= 0): Amount of bytes read.
 *   - (< 0): An error occurred.
 */
int64_t uk_9p_read(struct uk_9pdev *dev, struct uk_9pfid *fid,
		uint64_t offset, uint32_t count, char *buf);

/**
 * Writes count bytes from buf to the fid, starting from the given offset.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param fid
 *   9P fid to write to.
 * @param offset
 *   Offset at which to start writing.
 * @param count
 *   Maximum number of bytes to write.
 * @param buf
 *   Data to be written.
 * @return
 *   - (>= 0): Amount of bytes written.
 *   - (< 0): An error occurred.
 */
int64_t uk_9p_write(struct uk_9pdev *dev, struct uk_9pfid *fid,
		uint64_t offset, uint32_t count, const char *buf);

/**
 * Stats the given fid and places the data into the given stat structure.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param fid
 *   9P fid to stat.
 * @param stat
 *   Where to store the stat results.
 * @return
 *   - (!ERRPTR): The request. It must be removed only after all accesses to
 *   the strings in the stat structure are over.
 *   - ERRPTR: The error returned either by the API or by the remote server.
 */
struct uk_9preq *uk_9p_stat(struct uk_9pdev *dev, struct uk_9pfid *fid,
		struct uk_9p_stat *stat);

/**
 * Changes the file attributes of a given fid.
 *
 * @param dev
 *   The Unikraft 9P Device.
 * @param fid
 *   9P fid to clunk.
 * @return
 *   - 0: Successful.
 *   - (< 0): An error occurred.
 */
int uk_9p_wstat(struct uk_9pdev *dev, struct uk_9pfid *fid,
		struct uk_9p_stat *stat);

#ifdef __cplusplus
}
#endif

#endif /* __UK_9P__ */
