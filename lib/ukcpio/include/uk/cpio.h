/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Robert Hrusecky <roberth@cs.utexas.edu>
 *          Omar Jamil <omarj2898@gmail.com>
 *          Sachin Beldona <sachinbeldona@utexas.edu>
 *
 * Copyright (c) 2017, The University of Texas at Austin. All rights reserved.
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
#ifndef __UK_CPIO_H__
#define __UK_CPIO_H__

#include <stddef.h>

#ifdef __cplusplus
extern C {
#endif /* __cplusplus */

/**
 * Include also the case of unsupported headers
 */
enum ukcpio_error {
	UKCPIO_SUCCESS = 0,
	UKCPIO_INVALID_HEADER,
	UKCPIO_FILE_CREATE_FAILED,
	UKCPIO_FILE_WRITE_FAILED,
	UKCPIO_FILE_CHMOD_FAILED,
	UKCPIO_FILE_CLOSE_FAILED,
	UKCPIO_MKDIR_FAILED,
	UKCPIO_SYMLINK_FAILED,
	UKCPIO_MALFORMED_INPUT,
	UKCPIO_NODEST
};

/**
 * Extracts the given CPIO buffer to the path destination.
 *
 * @param dest
 *  The path location where the buffer will be extracted to.
 * @param buf
 *  A pointer to the first header of the CPIO buffer.
 * @param buflen
 *  The size of the CPIO buffer.
 * @return
 *  Returns 0 on success or one of ukcpio_error enums.
 */
enum ukcpio_error
ukcpio_extract(const char *dest, void *buf, size_t buflen);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CPIO_H__ */
