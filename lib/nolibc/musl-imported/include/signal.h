/* adapted from OSv */
#ifndef __UK_SIGNAL_H__
#define __UK_SIGNAL_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


#define SIG_BLOCK    0
#define SIG_UNBLOCK  1
#define SIG_SETMASK  2
#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGIOT    SIGABRT
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGSTKFLT 16
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGURG    23
#define SIGXCPU   24
#define SIGXFSZ   25
#define SIGVTALRM 26
#define SIGPROF   27
#define SIGWINCH  28
#define SIGIO     29
#define SIGPOLL   29
#define SIGPWR    30
#define SIGSYS    31
#define SIGUNUSED SIGSYS

#define _NSIG 32

#define SA_NOCLDSTOP  1
#define SA_NOCLDWAIT  2
#define SA_SIGINFO    4
#define SA_ONSTACK    0x08000000
#define SA_RESTART    0x10000000
#define SA_NODEFER    0x40000000
#define SA_RESETHAND  0x80000000
#define SA_RESTORER   0x04000000

#define __NEED_pid_t
#define __NEED_sig_atomic_t
#define __NEED_sigset_t
#include <nolibc-internal/shareddefs.h>

#define NSIG _NSIG

typedef struct {
	int          si_signo;    /* Signal number */
	int          si_code;     /* Cause of the signal */
	pid_t	       si_pid;	    /* Sending process ID */
} siginfo_t;

struct sigaction {
	union {
		void (*sa_handler)(int);
		void (*sa_sigaction)(int, siginfo_t *, void *);
	} __sa_handler;
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)(void);
};
#define sa_handler   __sa_handler.sa_handler
#define sa_sigaction __sa_handler.sa_sigaction

#define SIG_ERR  ((void (*)(int))-1)
#define SIG_DFL  ((void (*)(int)) 0)
#define SIG_IGN  ((void (*)(int)) 1)

/* TODO: do we have gnu statement expression? */
#define is_sig_dfl(ptr)	\
	(!((ptr)->sa_flags & SA_SIGINFO) && (ptr)->sa_handler == SIG_DFL)

#define is_sig_ign(ptr)	\
	(!((ptr)->sa_flags & SA_SIGINFO) && (ptr)->sa_handler == SIG_IGN)

int sigaction(int signum, const struct sigaction *act,
	      struct sigaction *oldact);

int sigpending(sigset_t *set);
int sigprocmask(int how, const sigset_t *set,
		sigset_t *oldset);
int sigsuspend(const sigset_t *mask);
int sigwait(const sigset_t *set, int *sig);

int kill(pid_t pid, int sig);
int killpg(int pgrp, int sig);
int raise(int sig);
int siginterrupt(int sig, int flag);
void psignal(int sig, const char *s);

typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
int sigismember(const sigset_t *set, int signo);

/* TODO: not used - defined just for newlib */
union sigval {
	int    sival_int;	/* Integer signal value */
	void  *sival_ptr;	/* Pointer signal value */
};

struct sigevent {
	int              sigev_notify;	/* Notification type */
	int              sigev_signo;	/* Signal number */
	union sigval     sigev_value;	/* Signal value */
};

/* TODO: not used - defined just for v8 */
typedef struct sigaltstack {
	void *ss_sp;
	int ss_flags;
	size_t ss_size;
} stack_t;

#ifdef __cplusplus
}
#endif

#endif /* __UK_SIGNAL_H__ */
