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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <vfscore/file.h>
#include <uk/plat/console.h>
#include <uk/essentials.h>
#include <termios.h>

/* One function for stderr and stdout */
static ssize_t stdout_write(struct vfscore_file *vfscore_file __unused,
			    const void *buf, size_t count)
{
	return ukplat_coutk(buf, count);
}

static ssize_t stdin_read(struct vfscore_file *vfscore_file __unused,
			  void *_buf, size_t count)
{
	int bytes_read;
	size_t bytes_total = 0;
	char *buf = (char *)_buf;

	do {
		while ((bytes_read = ukplat_cink(buf,
			count - bytes_total)) <= 0)
			;

		buf = buf + bytes_read;
		*(buf - 1) = *(buf - 1) == '\r' ?
					'\n' : *(buf - 1);

		stdout_write(vfscore_file, (buf - bytes_read),
				bytes_read);
		bytes_total += bytes_read;

	} while (bytes_total < count && *(buf - 1) != '\n'
			&& *(buf - 1) != VEOF);

	return bytes_total;
}

static struct vfscore_fops stdin_fops = {
	.read = stdin_read,
};

static struct vfscore_fops stdout_fops = {
	.write = stdout_write,
};

static struct vfscore_file  stdin_file = {
	.fd = 0,
	.fops = &stdin_fops,
};

static struct vfscore_file  stdout_file = {
	.fd = 1,
	.fops = &stdout_fops,
};

static struct vfscore_file  stderr_file = {
	.fd = 2,
	.fops = &stdout_fops,
};


void init_stdio(void)
{
	vfscore_install_fd(0, &stdin_file);
	vfscore_install_fd(1, &stdout_file);
	vfscore_install_fd(2, &stderr_file);
}
