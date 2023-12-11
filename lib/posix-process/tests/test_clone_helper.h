#include <uk/test.h>
#include <uk/syscall.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <uk/print.h>
#include <uk/syscall.h>
#include <uk/sched.h>

#define CSIGNAL			0x000000ff
#define CLONE_VM		0x00000100
#define CLONE_FS		0x00000200
#define CLONE_FILES		0x00000400
#define CLONE_SIGHAND		0x00000800
#define CLONE_PTRACE		0x00002000
#define CLONE_VFORK		0x00004000
#define CLONE_PARENT		0x00008000
#define CLONE_THREAD		0x00010000
#define CLONE_NEWNS		0x00020000
#define CLONE_SYSVSEM		0x00040000
#define CLONE_SETTLS		0x00080000
#define CLONE_PARENT_SETTID	0x00100000
#define CLONE_CHILD_CLEARTID	0x00200000
#define CLONE_DETACHED		0x00400000
#define CLONE_UNTRACED		0x00800000
#define CLONE_CHILD_SETTID	0x01000000
#define CLONE_NEWCGROUP		0x02000000
#define CLONE_NEWUTS		0x04000000
#define CLONE_NEWIPC		0x08000000
#define CLONE_NEWUSER		0x10000000
#define CLONE_NEWPID		0x20000000
#define CLONE_NEWNET		0x40000000
#define CLONE_IO		0x80000000

#define syscall(...) uk_syscall_r_static(__VA_ARGS__)

int __clone(int (*func)(void *), void *stack, int flags, void *arg, ...);
void *__uk_copy_tls(unsigned char *mem);

void test_clone(void);
void test_pthread_basic(void);
void test_pthread_detached(void);

static inline int __check_if_child_exited(struct uk_thread *thread_ptr)
{
	struct uk_sched *scheduler = uk_sched_current();
	struct uk_thread *itr;

	/* The function didn't even run*/
	if (thread_ptr == NULL)
		return 0;

	/* The child is still in the scheduler's list */
	UK_TAILQ_FOREACH(itr, &scheduler->thread_list, thread_list) {
		if (itr == thread_ptr)
			return 0;
	}

	return 1;
}
