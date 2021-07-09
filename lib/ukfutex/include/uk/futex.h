#ifndef __UK_ATOMIC_H__
#define __UK_ATOMIC_H__

#include <time.h>
#include <uk/time_types.h>
#include <uk/sched.h>
#include <uk/list.h>
#include <uk/assert.h>

#define FUTEX_WAIT		0
#define FUTEX_WAKE		1
#define FUTEX_FD        2
#define FUTEX_REQUEUE		3
#define FUTEX_CMP_REQUEUE	4
#define FUTEX_WAKE_OP		5
#define FUTEX_LOCK_PI		6
#define FUTEX_UNLOCK_PI		7
#define FUTEX_TRYLOCK_PI	8
#define FUTEX_WAIT_BITSET	9
#define FUTEX_WAKE_BITSET	10
#define FUTEX_WAIT_REQUEUE_PI	11
#define FUTEX_CMP_REQUEUE_PI	12

struct uk_futex {
	uint32_t *futex;
	struct uk_thread *thread;
	struct uk_list_head list_node;
};

long futex(uint32_t *uaddr, int futex_op, uint32_t val,
			const struct timespec *timeout,
			uint32_t *uaddr2, uint32_t val3);

#endif
