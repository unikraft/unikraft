/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __POSIX_UNIXSOCKET_RIGHTS_H__
#define __POSIX_UNIXSOCKET_RIGHTS_H__

#include <sys/socket.h>
#include <uk/alloc.h>
#include <uk/posix-pipe.h>

int unix_rights_prepare(struct uk_alloc *alloc, const struct msghdr *msg,
			struct uk_pipe_control **control);

void unix_rights_receive(struct msghdr *msg, int flags,
			 struct uk_pipe_control *control);

#endif /* __POSIX_UNIXSOCKET_RIGHTS_H__ */
