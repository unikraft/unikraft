/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <sys/signalfd.h>
#include <poll.h>

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/errptr.h>
#include <uk/file/iovutil.h>
#include <uk/file/nops.h>

#if CONFIG_LIBPOSIX_FDTAB
#include <uk/posix-fd.h>
#include <uk/posix-fdtab.h>
#include <uk/syscall.h>
#endif /* CONFIG_LIBPOSIX_FDTAB */

#include <signal.h>

#include "signal.h"

/* Volume ID string used to identify signal files */
static const char SIGNAL_VOLID[] = "signal_vol";
/* Macro to check if a file is a signal file by comparing volume ID */
#define FILE_IS_SIGNAL(f)						\
	((f)->vol == SIGNAL_VOLID)

static inline
struct uk_signal_file *uk_file_to_signal_file(const struct uk_file *f)
{
	UK_ASSERT(f);
	UK_ASSERT(f->node);
	UK_ASSERT(FILE_IS_SIGNAL(f));
	return (struct uk_signal_file *)f->node;
}

static uk_pollevent signal_file_poll(const struct uk_file *f, uk_pollevent mask)
{
	const struct uk_signal_file *sigf;
	struct posix_thread *pthread;
	struct posix_process *pproc;
	__sz sn;

	if (unlikely(!(mask & UKFD_POLLIN)))
		return 0;

	sigf = uk_file_to_signal_file(f);
	pthread = uk_pthread_current();
	pproc = uk_pprocess_current();

	pprocess_signal_foreach(sn)
		if (uk_sigismember(&sigf->mask, sn) &&
		    (IS_PENDING(pthread->signal->sigqueue, sn) ||
		     IS_PENDING(pproc->signal->sigqueue, sn)))
			return UKFD_POLLIN;

	return 0;
}

/* Convert internal siginfo_t to signalfd_siginfo structure */
static void si_to_ssi(const siginfo_t *si, struct signalfd_siginfo *ssi)
{
	/* Zero out the structure first */
	memset(ssi, 0, sizeof(*ssi));

	/* Basic signal information */
	ssi->ssi_signo = si->si_signo;
	ssi->ssi_errno = si->si_errno;
	ssi->ssi_code = si->si_code;

	/* Sender identification */
	ssi->ssi_pid = si->si_pid;
	ssi->ssi_uid = si->si_uid;

	/* Signal-specific fields */
	switch (si->si_signo) {
	case SIGCHLD:
		ssi->ssi_status = si->si_status;
		ssi->ssi_utime = si->si_utime;
		ssi->ssi_stime = si->si_stime;
		break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
		ssi->ssi_addr = (uint64_t)si->si_addr;
		ssi->ssi_addr_lsb = si->si_addr_lsb;
		break;
	case SIGIO:
		ssi->ssi_band = si->si_band;
		ssi->ssi_fd = si->si_fd;
		break;
	default:
		/* For real-time signals which may contain sigqueue data */
		if (si->si_signo >= SIGRTMIN && si->si_signo <= SIGRTMAX) {
			ssi->ssi_int = si->si_value.sival_int;
			ssi->ssi_ptr = (uint64_t)si->si_value.sival_ptr;
			return;
		}

		break;
	}

	/* Handle signals sent with sigqueue regardless of signal number */
	if (si->si_code == SI_QUEUE || si->si_code == SI_USER) {
		ssi->ssi_int = si->si_value.sival_int;
		ssi->ssi_ptr = (uint64_t)si->si_value.sival_ptr;
	}
}

static ssize_t signal_file_read(const struct uk_file *f,
				const struct iovec *iov, size_t iovcnt,
				size_t off, long flags __unused)
{
	size_t iovlen_total, iovlen_wr, iovi, cur;
	struct posix_thread *pthread;
	struct posix_process *pproc;
	struct uk_signal_file *sigf;
	struct signalfd_siginfo ssi;
	struct uk_signal *sig;
	__sz sn;

	if (unlikely(off))
		return -EINVAL;

	if (unlikely(!iovcnt))
		return -EINVAL;

	if (unlikely(!iov[0].iov_base))
		return -EFAULT;

	iovlen_total = uk_iov_remaining(iov, iovcnt, 0, 0);
	if (unlikely(iovlen_total < sizeof(ssi)))
		return -EINVAL;

	sigf = uk_file_to_signal_file(f);

	pthread = uk_pthread_current();
	pproc = uk_pprocess_current();

	iovlen_wr = 0;
	iovi = 0;
	cur = 0;
	pprocess_signal_foreach(sn) {
		if (!uk_sigismember(&sigf->mask, sn))
			continue;

		/*
		 * Proceed like Linux and always prioritize dequeueing
		 * signal from the specific target thread and only fallback
		 * to dequeueing from the containing process if it also happens
		 * to have this signal queued.
		 */
		if (IS_PENDING(pthread->signal->sigqueue, sn))
			sig = pprocess_signal_dequeue(pproc, pthread, sn);
		else if (IS_PENDING(pproc->signal->sigqueue, sn))
			sig = pprocess_signal_dequeue(pproc, NULL, sn);
		else
			continue;

		if (!sig)
			continue;

		si_to_ssi(&sig->siginfo, &ssi);
		uk_signal_free(pthread->_a, sig);
		iovlen_wr += uk_iov_scatter(iov, iovcnt,
					    (const char *)&ssi, sizeof(ssi),
					    &iovi, &cur);

		/* Have we reached the end? */
		if (iovlen_total - iovlen_wr < sizeof(ssi))
			break;
	}

	/* No pending signals were queued - try again */
	if (!iovlen_wr)
		return -EAGAIN;

	return iovlen_wr;
}

static const struct uk_file_ops signal_file_ops = {
	.read = signal_file_read,
	.write = uk_file_nop_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl
};

static void signal_file_release(const struct uk_file *f, int what)
{
	struct posix_process *pproc;
	struct uk_signal_file *sigf;

	pproc = uk_pprocess_current();

	if (what & UK_FILE_RELEASE_OBJ) {
		/* Free */
		sigf = uk_file_to_signal_file(f);
		pprocess_signal_file_del(pproc, sigf);
		uk_free(sigf->a, sigf);
	}
}

const struct uk_file *signal_file_create(struct uk_alloc *a,
					 const uk_sigset_t *mask)
{
	struct uk_signal_file *sigf;
	struct posix_process *pproc;

	sigf = uk_malloc(a, sizeof(*sigf));
	if (unlikely(!sigf))
		return ERR2PTR(-ENOMEM);

	sigf->a = a;
	sigf->fstate = UK_FILE_POLLED_STATE_INIT_VALUE(sigf->fstate,
						       &signal_file_poll);
	sigf->frefcnt = UK_FILE_REFCNT_INIT_VALUE(sigf->frefcnt);

	sigf->f = (struct uk_file){
		.vol = SIGNAL_VOLID,
		.node = sigf,
		.refcnt = &sigf->frefcnt,
		.state = &sigf->fstate,
		.ops = &signal_file_ops,
		._release = signal_file_release,
	};

	/*
	 * It is not possible to receive SIGKILL or SIGSTOP signals via a
	 * signal file descriptor; these signals are silently ignored if
	 * specified in mask.
	 */
	uk_sigcopyset(&sigf->mask, mask);
	uk_sigdelset(&sigf->mask, SIGKILL);
	uk_sigdelset(&sigf->mask, SIGSTOP);

	pproc = uk_pprocess_current();
	pprocess_signal_file_add(pproc, sigf);

	return &sigf->f;
}

#if CONFIG_LIBPOSIX_FDTAB
static int signalfd_create(const uk_sigset_t *mask, int flags)
{
	unsigned int mode = O_RDONLY | UKFD_O_NOSEEK;
	const struct uk_file *sigf;
	int fd;

	sigf = signal_file_create(uk_alloc_get_default(), mask);
	if (unlikely(PTRISERR(sigf)))
		return PTR2ERR(sigf);

	/* Register fd */
	if (flags & SFD_NONBLOCK)
		mode |= O_NONBLOCK;
	if (flags & SFD_CLOEXEC)
		mode |= O_CLOEXEC;

	fd = uk_fdtab_open(sigf, mode);
	uk_file_release(sigf);

	return fd;
}

static int signalfd_set_mask(int fd, const uk_sigset_t *new_mask)
{
	struct uk_signal_file *sigf;
	struct posix_process *pproc;
	struct uk_ofile *of;

	pproc = uk_pprocess_current();
	UK_ASSERT(pproc);

	of = uk_fdtab_get(fd);
	if (unlikely(!of))
		return -EBADF;

	sigf = uk_file_to_signal_file(of->file);

	/*
	 * It is not possible to receive SIGKILL or SIGSTOP signals via a
	 * signal file descriptor; these signals are silently ignored if
	 * specified in mask.
	 */
	uk_sigcopyset(&sigf->mask, new_mask);
	uk_sigdelset(&sigf->mask, SIGKILL);
	uk_sigdelset(&sigf->mask, SIGSTOP);

	/* Make sure we don't miss any newly added signals */
	uk_sigorset(&pproc->signal->sigfiles_ctx.allmask, &sigf->mask);

	uk_ofile_release(of);

	return 0;
}

static inline int uk_sys_signalfd(int fd, const uk_sigset_t *mask,
				  size_t masksz, int flags)
{
	if (unlikely(masksz != sizeof(uk_sigset_t)))
		return -EINVAL;

	if (unlikely(!mask))
		return -EFAULT;

	if (fd == -1)
		return signalfd_create(mask, flags);

	return signalfd_set_mask(fd, mask);
}

UK_LLSYSCALL_R_DEFINE(int, signalfd4,
		      int, fd,
		      const uk_sigset_t *, mask,
		      size_t, masksz,
		      int, flags)
{
	return uk_sys_signalfd(fd, mask, masksz, flags);
}

#if UK_LIBC_SYSCALLS
int signalfd(int fd, const sigset_t *mask, int flags)
{
	/*
	 * Libc implementations define a larger, extensible representation
	 * (usually 128 bytes) of `sigset_t` to comply with POSIX standards
	 * and accommodate potential future expansions. However,
	 * kernels like Linux typically have a smaller defined `sigset_t` of
	 * a fixed size of 8 bytes as that is what it uses at the time of
	 * writing this. When a libc makes syscalls that use signal masks it
	 * truncates or extracts only the relevant portion of its internal
	 * representation to pass to the kernel. This helps ensure compatibility
	 * between userspace applications and the kernel.
	 *
	 * Thus, mimic a typical libc here and pass a hardcoded value: that
	 * of the number of signals supported divided by the number of bits in
	 * a byte.
	 */
	return (int)uk_syscall_e_signalfd4((long)fd, (long)mask,
					   NSIG / 8, (long)flags);
}
#endif /* UK_LIBC_SYSCALLS */

UK_LLSYSCALL_R_DEFINE(int, signalfd,
		      int, fd,
		      const uk_sigset_t *, mask,
		      size_t, masksz)
{
	return uk_sys_signalfd(fd, mask, masksz, 0);
}
#endif /* CONFIG_LIBPOSIX_FDTAB */
