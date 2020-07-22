/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 */

#include <errno.h>
#include <stdio.h>
#include <uk/print.h>
#include <uk/arch/types.h>
#include <linuxu/tap.h>

int tap_open(__u32 flags)
{
	int rc = 0;

	rc = sys_open(TAPDEV_PATH, flags);
	if (rc < 0)
		uk_pr_err("Error in opening the tap device\n");
	return rc;
}

int tap_dev_configure(int fd, __u32 feature_flags, void *arg)
{
	int rc = 0;
	struct uk_ifreq *ifreq = (struct uk_ifreq *) arg;

	/* Set the tap device configuration */
	ifreq->ifr_flags = UK_IFF_TAP | UK_IFF_NO_PI | feature_flags;
	if ((rc = sys_ioctl(fd, UK_TUNSETIFF, ifreq)) < 0)
		uk_pr_err("Failed(%d) to configure the tap device\n", rc);

	return rc;
}

int tap_netif_configure(int fd, __u32 request, void *arg)
{
	int rc;
	struct uk_ifreq *usr_ifr = (struct uk_ifreq *) arg;
	struct uk_ifreq ifr = {0};

	switch (request) {
	case UK_SIOCSIFFLAGS:
		snprintf(ifr.ifr_name, IFNAMSIZ, "%s", usr_ifr->ifr_name);
		rc = sys_ioctl(fd, UK_SIOCGIFFLAGS, &ifr);
		/* fetch current flags to leave other flags untouched */
		if (rc < 0) {
			uk_pr_err("Failed to read flags %d\n", rc);
			goto exit_error;
		}
		usr_ifr->ifr_flags |= ifr.ifr_flags;
		break;
	case UK_SIOCGIFFLAGS:
	case UK_SIOCGIFINDEX:
	case UK_SIOCGIFHWADDR:
	case UK_SIOCSIFHWADDR:
	case UK_SIOCSIFMTU:
	case UK_SIOCGIFMTU:
	case UK_SIOCSIFTXQLEN:
	case UK_SIOCGIFTXQLEN:
	case UK_SIOCBRADDIF:
		break;
	default:
		rc = -EINVAL;
		uk_pr_err("Invalid ioctl request\n");
		goto exit_error;
	}

	if ((rc = sys_ioctl(fd, request, usr_ifr)) < 0) {
		uk_pr_err("Failed to set device control %d\n", rc);
		goto exit_error;
	}

	return 0;

exit_error:
	return rc;
}

int tap_netif_create(void)
{
	return sys_socket(AF_INET, SOCK_DGRAM, 0);
}

ssize_t tap_read(int fd, void *buf, size_t count)
{
	ssize_t rc = -EINTR;

	while (rc == -EINTR)
		rc = sys_read(fd, buf, count);

	if (rc == -11)
		/* Explicitly added since linux errno has -11 for EAGAIN */
		rc = -EWOULDBLOCK;
	else if (rc < 0)
		uk_pr_err("Failed(%ld) to read from the tap device\n", rc);

	return rc;
}

ssize_t tap_write(int fd, const void *buf, size_t count)
{
	ssize_t rc = -EINTR;
	size_t written = 0;

	while (count > 0) {
		rc = sys_write(fd, buf + written, count);
		if (rc == -EINTR)
			continue;
		else if (rc == -11) {
			/*
			 * Explicitly added since linux errno has -11 for
			 * EAGAIN.
			 * FIXME: EAGAIN is not the only error code affected by
			 * this issue (eg EDESTADDRREQ, EDQUOT). We should think
			 * of a more generic solution to address it.
			 */
			rc = -EAGAIN;
		} else if (rc < 0) {
			uk_pr_err("Failed(%ld) to write to the tap device\n",
				  rc);
			return rc;
		}
		count -= rc;
		written += rc;
	}
	return (ssize_t)written;
}

int tap_close(int fd)
{
	return sys_close(fd);
}
