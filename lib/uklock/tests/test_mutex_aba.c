/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdbool.h>
#include <stddef.h>
#include <uk/crash.h>
#include <uk/essentials.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/mutex.h>
#include <uk/pcpuvar.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/wait.h>

#include "../mutex_test.h"

#define MUTEX_ABA_ROUNDS 8U
#define MUTEX_ABA_TIMEOUT ((__nsec)2000000000ULL)

struct mutex_aba_probe {
	struct uk_mutex mutex;
	struct uk_semaphore start;
	struct uk_semaphore marked;
	struct uk_semaphore resume;
	struct uk_semaphore reaped;
	struct uk_thread *waiter;
	unsigned int waiter_cpu;
	unsigned int round;
	unsigned int hook_entries;
	bool armed;
	bool blocked;
	bool acquired;
};

static struct mutex_aba_probe mutex_aba_state;

static void aba_require(bool condition, const char *stage)
{
	struct mutex_aba_probe *p = &mutex_aba_state;

	if (condition)
		return;
	uk_pr_err("MUTEX_ABA_FAIL stage=%s round=%u\n", stage, p->round);
	uk_pr_err("MUTEX_ABA_STATE marked=%u blocked=%u acquired=%u\n",
		  p->hook_entries, p->blocked,
		  __atomic_load_n(&p->acquired, __ATOMIC_ACQUIRE));
	UK_CRASH("Mutex owner-cycle progress contract failed\n");
}

static void aba_wait(struct uk_semaphore *sem, const char *stage)
{
	__nsec elapsed = uk_semaphore_down_to(sem, MUTEX_ABA_TIMEOUT);

	aba_require(elapsed < MUTEX_ABA_TIMEOUT / 2, stage);
}

void uk_mutex_test_after_mark(struct uk_mutex *m)
{
	struct mutex_aba_probe *p = &mutex_aba_state;
	bool armed = true;

	if (m != &p->mutex || uk_thread_current() != p->waiter ||
	    !__atomic_compare_exchange_n(&p->armed, &armed, false, 0,
					 __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
		return;
	aba_require(__atomic_load_n(&m->lock, __ATOMIC_ACQUIRE) &
		    _UK_MUTEX_CONTESTED, "marked state");
	p->hook_entries++;
	uk_semaphore_up(&p->marked);
	aba_wait(&p->resume, "owner cycle");
}

static void aba_reaped(struct uk_thread *thread __unused, void *arg)
{
	struct mutex_aba_probe *p = arg;

	uk_semaphore_up(&p->reaped);
}

static void aba_waiter(void *arg) __noreturn;

static void aba_waiter(void *arg)
{
	struct mutex_aba_probe *p = arg;

	aba_wait(&p->start, "worker start");
	aba_require(uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx) ==
		    p->waiter_cpu, "worker CPU");
	uk_mutex_lock(&p->mutex);
	__atomic_store_n(&p->acquired, true, __ATOMIC_RELEASE);
	uk_mutex_unlock(&p->mutex);
	uk_sched_thread_exit2(aba_reaped, p);
}

static bool aba_waiter_blocked(struct mutex_aba_probe *p)
{
	unsigned long irqf;
	bool blocked;

	/* The waiter registers and blocks while holding this queue lock. */
	ukplat_spin_lock_irqsave(&p->mutex.wait.lock, irqf);
	blocked = p->waiter->wait_ticket.wq == &p->mutex.wait &&
		!uk_thread_is_runnable(p->waiter);
	ukplat_spin_unlock_irqrestore(&p->mutex.wait.lock, irqf);
	return blocked;
}

static struct uk_sched *aba_scheduler(unsigned int cpu)
{
	struct uk_sched *local = uk_sched_current();
	struct uk_sched *candidate;
	unsigned int tries;

	if (local->lcpu_idx == (int)cpu)
		return local;
	for (tries = 0; tries < 16 * CONFIG_UKPLAT_CPU_MAXCOUNT; tries++) {
		candidate = uk_sched_select_round_robin(local);
		if (candidate && candidate->lcpu_idx == (int)cpu)
			return candidate;
	}
	return NULL;
}

static void aba_round(struct uk_sched *target, unsigned int cpu,
		      unsigned int round)
{
	struct mutex_aba_probe *p = &mutex_aba_state;
	__nsec deadline;

	memset_isr(p, 0, sizeof(*p));
	p->waiter_cpu = cpu;
	p->round = round;
	p->armed = true;
	uk_mutex_init(&p->mutex);
	uk_semaphore_init(&p->start, 0);
	uk_semaphore_init(&p->marked, 0);
	uk_semaphore_init(&p->resume, 0);
	uk_semaphore_init(&p->reaped, 0);
	uk_mutex_lock(&p->mutex);
	p->waiter = uk_sched_thread_create_fn1(target, aba_waiter,
					       p, 0, 0, false, false,
					       "mutex-aba", NULL, NULL);
	aba_require(p->waiter, "create waiter");
	uk_semaphore_up(&p->start);
	aba_wait(&p->marked, "wait for marking");

	/* Restore the exact unmarked owner value seen before CAS. */
	uk_mutex_unlock(&p->mutex);
	uk_mutex_lock(&p->mutex);
	aba_require(!(p->mutex.lock & _UK_MUTEX_STATE_MASK),
		    "unmarked reacquisition");
	uk_semaphore_up(&p->resume);
	deadline = ukplat_monotonic_clock() + MUTEX_ABA_TIMEOUT;
	while (!(p->blocked = aba_waiter_blocked(p)) &&
	       ukplat_monotonic_clock() < deadline)
		uk_sched_yield();
	aba_require(p->blocked, "waiter queued");
	uk_mutex_unlock(&p->mutex);
	aba_wait(&p->reaped, "completion");
	aba_require(p->hook_entries == 1 &&
		    __atomic_load_n(&p->acquired, __ATOMIC_ACQUIRE),
		    "owner cycle completed");
}

static void __constructor run_mutex_aba_probe(void)
{
	unsigned int cpus = uk_sched_started_count();
	unsigned int first = cpus > 1 ? 1 : 0;
	unsigned int cpu, round, cycles = 0;
	struct uk_sched *target;

	aba_require(uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx) == 0,
		    "owner CPU");
	for (cpu = first; cpu < cpus; cpu++) {
		target = aba_scheduler(cpu);
		aba_require(target, "find scheduler");
		for (round = 0; round < MUTEX_ABA_ROUNDS; round++) {
			aba_round(target, cpu, round);
			cycles++;
		}
		uk_pr_info("MUTEX_ABA_PAIR owner=0 waiter=%u cycles=%u\n",
			   cpu, MUTEX_ABA_ROUNDS);
	}
	uk_pr_info("MUTEX_ABA_PASS cpus=%u pairs=%u cycles=%u\n",
		   cpus, cpus - first, cycles);
}
