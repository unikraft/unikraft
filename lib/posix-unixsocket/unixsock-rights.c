/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <uk/essentials.h>

#include "unixsock-rights.h"

#if CONFIG_LIBPOSIX_FDTAB
#include <uk/posix-fdtab.h>

#define UNIX_RIGHTS_MAX 253

struct unix_rights {
	struct uk_pipe_control control;
	struct uk_alloc *alloc;
	size_t count;
	struct uk_ofile *files[];
};

static void unix_rights_release(struct uk_pipe_control *control)
{
	struct unix_rights *rights = __containerof(control, struct unix_rights,
						    control);

	for (size_t i = 0; i < rights->count; i++) {
		if (rights->files[i])
			uk_ofile_release(rights->files[i]);
	}
	uk_free(rights->alloc, rights);
}

int unix_rights_prepare(struct uk_alloc *alloc, const struct msghdr *msg,
			struct uk_pipe_control **control)
{
	size_t remaining = msg->msg_controllen, capacity;
	const char *data = msg->msg_control;
	struct unix_rights *rights;
	int ret = -EINVAL;

	*control = NULL;
	if (!remaining)
		return 0;
	if (!data)
		return -EFAULT;
	if (remaining < CMSG_LEN(0))
		return -EINVAL;

	capacity = MIN(UNIX_RIGHTS_MAX,
		       (remaining - CMSG_LEN(0)) / sizeof(int));
	rights = uk_malloc(alloc, sizeof(*rights) +
			   capacity * sizeof(rights->files[0]));
	if (!rights)
		return -ENOMEM;
	*rights = (struct unix_rights){
		.control.release = unix_rights_release,
		.alloc = alloc,
	};

	while (remaining >= CMSG_LEN(0)) {
		struct cmsghdr header;
		size_t bytes, count, span;

		memcpy(&header, data, sizeof(header));
		if (header.cmsg_len < CMSG_LEN(0) ||
		    header.cmsg_len > remaining)
			goto error;
		if (header.cmsg_level != SOL_SOCKET ||
		    header.cmsg_type != SCM_RIGHTS) {
			ret = -EOPNOTSUPP;
			goto error;
		}
		bytes = header.cmsg_len - CMSG_LEN(0);
		if (bytes % sizeof(int))
			goto error;
		count = bytes / sizeof(int);
		if (count > capacity - rights->count)
			goto error;
		for (size_t i = 0; i < count; i++) {
			struct uk_ofile *file;
			int fd;

			memcpy(&fd, data + CMSG_LEN(0) + i * sizeof(fd),
			       sizeof(fd));
			file = uk_fdtab_get(fd);
			if (!file) {
				ret = -EBADF;
				goto error;
			}
			rights->files[rights->count++] = file;
		}

		span = CMSG_ALIGN(header.cmsg_len);
		if (span < header.cmsg_len || span >= remaining)
			break;
		data += span;
		remaining -= span;
	}

	if (!rights->count) {
		unix_rights_release(&rights->control);
		return 0;
	}
	*control = &rights->control;
	return 0;

error:
	unix_rights_release(&rights->control);
	return ret;
}

void unix_rights_receive(struct msghdr *msg, int flags,
			 struct uk_pipe_control *control)
{
	size_t remaining = msg->msg_controllen;
	char *data = msg->msg_control;
	struct unix_rights *rights;
	size_t capacity = 0, count = 0;
	unsigned int mode = flags & MSG_CMSG_CLOEXEC ? O_CLOEXEC : 0;

	msg->msg_controllen = 0;
	if (!control)
		return;

	rights = __containerof(control, struct unix_rights, control);
	if (data && remaining >= CMSG_LEN(0))
		capacity = (remaining - CMSG_LEN(0)) / sizeof(int);
	while (count < rights->count && count < capacity) {
		int fd = uk_fdtab_open_desc(rights->files[count], mode);

		if (fd < 0)
			break;
		rights->files[count] = NULL;
		memcpy(data + CMSG_LEN(0) + count * sizeof(fd), &fd,
		       sizeof(fd));
		count++;
	}

	if (count) {
		size_t bytes = count * sizeof(int);
		struct cmsghdr header = {
			.cmsg_len = CMSG_LEN(bytes),
			.cmsg_level = SOL_SOCKET,
			.cmsg_type = SCM_RIGHTS,
		};

		memcpy(data, &header, sizeof(header));
		msg->msg_controllen = MIN(remaining, CMSG_SPACE(bytes));
	}
	if (count != rights->count)
		msg->msg_flags |= MSG_CTRUNC;
	unix_rights_release(control);
}

#else /* !CONFIG_LIBPOSIX_FDTAB */

int unix_rights_prepare(struct uk_alloc *alloc __unused,
			const struct msghdr *msg,
			struct uk_pipe_control **control)
{
	*control = NULL;
	return msg->msg_controllen ? -EOPNOTSUPP : 0;
}

void unix_rights_receive(struct msghdr *msg, int flags __unused,
			 struct uk_pipe_control *control __unused)
{
	msg->msg_controllen = 0;
}

#endif /* CONFIG_LIBPOSIX_FDTAB */
