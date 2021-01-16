#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

void psignal(int sig, const char *msg)
{
	FILE *f = stderr;
	char *s = strsignal(sig);
	int old_errno = errno;


	/* Save stderr's orientation and encoding rule, since psignal is not
	 * permitted to change them. Save errno and restore it if there is no
	 * error since fprintf might change it even on success but psignal is
	 * not permitted to do so. */

	if (fprintf(f, "%s%s%s\n", msg?msg:"", msg?": ":"", s)>=0)
		errno = old_errno;
}
