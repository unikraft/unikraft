/* taken from newlib */

#include <errno.h>
#include <uk/uk_signal.h>
#include <signal.h>

int sigemptyset(sigset_t *set)
{
	uk_sigemptyset(set);
	return 0;
}

int sigfillset(sigset_t *set)
{
	uk_sigfillset(set);
	return 0;
}

int sigaddset(sigset_t *set, int signo)
{
	if (signo >= NSIG || signo <= 0) {
		errno = EINVAL;
		return -1;
	}

	uk_sigaddset(set, signo);
	return 0;
}

int sigdelset(sigset_t *set, int signo)
{
	if (signo >= NSIG || signo <= 0) {
		errno = EINVAL;
		return -1;
	}

	uk_sigdelset(set, signo);
	return 0;
}

int sigismember(const sigset_t *set, int signo)
{
	if (signo >= NSIG || signo <= 0) {
		errno = EINVAL;
		return -1;
	}

	if (uk_sigismember(set, signo))
		return 1;
	else
		return 0;
}
