/* Timer wheel implementation inspired by the documentation comments of the
 * Linux timer.c implementation
 */
#include "timer_wheel.h"

#include <uk/essentials.h>
#include <uk/list.h>
#include <uk/timer.h>
#include <uk/tick.h>
#include <uk/print.h>
#include <uk/trace.h>
#include <uk/bitops.h>

#define WHEEL_SHIFT			3

#define WHEEL_BUCKET_BITS		6
#define WHEEL_BUCKETS			(1 << WHEEL_BUCKET_BITS)
#define WHEEL_BUCKETS_MASK		(WHEEL_BUCKETS - 1)

#define WHEEL_LEVELS			8

#define WHEEL_LEVEL_GRAN(lvl)		((lvl) * WHEEL_SHIFT)
#define WHEEL_LEVEL_BUCKET_SEL(time, lvl)				\
        ((time) >> WHEEL_LEVEL_GRAN(lvl))
#define WHEEL_LEVEL_BUCKET_IDX(time, lvl)				\
        (WHEEL_LEVEL_BUCKET_SEL(time, lvl) & WHEEL_BUCKETS_MASK)

UK_CTASSERT(WHEEL_BUCKET_BITS % WHEEL_SHIFT == 0);

UK_TRACEPOINT(uktimer_wheel_timer_added,
	      "timer=%#x expiry=%lu lvl=%d bucket=%d", void*, __u64, int, int);

UK_TRACEPOINT(uktimer_wheel_timer_expired,
	      "timer=%#x delay=%lu", void*, __u64);

static struct uk_hlist_head timer_wheel[WHEEL_LEVELS][WHEEL_BUCKETS];

static __u64 next_event;

static __u64 uktimer_wheel_find_next_event(int level)
{
	struct uk_timer *timer;
	int bucket, end;

	bucket = WHEEL_LEVEL_BUCKET_SEL(uk_jiffies, level);
	end = bucket + WHEEL_BUCKETS;

	while (bucket != end) {
		timer = uk_hlist_entry_safe(
		    timer_wheel[level][bucket & WHEEL_BUCKETS_MASK].first,
		    struct uk_timer, timer_list);
		if (timer)
			return timer->expiry;
		bucket = (bucket + 1);
	}

	return 0;
}

__u64 uktimer_wheel_next_event(void)
{
	int level;

	if (!next_event) {
		for (level = 0; level < WHEEL_LEVELS; level++) {
			next_event = uktimer_wheel_find_next_event(level);
			if (next_event)
				break;
		}
	}
	return next_event;
}

void uktimer_wheel_add(struct uk_timer *timer)
{
	__u64 delta;
	int level, bucket;

	if (timer->registered)
		return;

	delta = timer->expiry > uk_jiffies ? timer->expiry - uk_jiffies : 0;

	level = delta
		    ? (uk_fls(delta) / WHEEL_SHIFT)
		    : 0;
	level += 1 - WHEEL_BUCKET_BITS / WHEEL_SHIFT;
	/* This ensure that timers which are too far in the future, will expire
	 * at the maximum supported point and are then re-added */
	level = MAX(MIN(level, WHEEL_LEVELS - 1), 0);

	bucket = WHEEL_LEVEL_BUCKET_IDX(timer->expiry, level);

	uktimer_wheel_timer_added(timer, timer->expiry, level, bucket);
	uk_hlist_add_head(&timer->timer_list, &timer_wheel[level][bucket]);

	next_event = MIN(next_event, timer->expiry);

	timer->registered = 1;
}

static void
uktimer_wheel_expire(__u64 to_jiffies, struct uk_hlist_head *expired)
{
	struct uk_hlist_node *tmp;
	struct uk_timer *timer;

	uk_hlist_for_each_entry_safe(timer, tmp, expired, timer_list) {
		timer->registered = 0;

		if (timer->expiry > uk_jiffies) {
			/* Expiry is still too early, which can happen if the
			 * expiry is too far in the future for the timer wheel
			 */
			uk_hlist_del(&timer->timer_list);
			uktimer_timer_add(timer);
			continue;
		}

		uktimer_wheel_timer_expired(timer, to_jiffies - timer->expiry);
		timer->callback(timer);
	}

	next_event = 0;
}

void uktimer_wheel_tick(__u64 ticks)
{
	int level;
	__u64 to_jiffies = uk_jiffies + ticks;
	int bucket, end;
	struct uk_hlist_head expire = UK_HLIST_HEAD_INIT;

	for (level = 0; level < WHEEL_LEVELS; level++) {
		bucket = WHEEL_LEVEL_BUCKET_SEL(uk_jiffies, level);
		end = WHEEL_LEVEL_BUCKET_SEL(to_jiffies, level);

		/* If there are no expiries possible on this level then the
		 * other levels also won't expire
		 */
		if (bucket == end)
			break;

		/* Do at most one iteration over all buckets */
		end = bucket + MIN(WHEEL_BUCKETS, end - bucket);

		while (bucket != end) {
			uk_hlist_move_list(
			    &timer_wheel[level][bucket & WHEEL_BUCKETS_MASK],
			    &expire);
			uktimer_wheel_expire(to_jiffies, &expire);
			bucket = (bucket + 1);
		}
	}
}
