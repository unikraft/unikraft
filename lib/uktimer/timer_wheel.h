#ifndef __UKTIMER_TIMER_WHEEL_H__
#define __UKTIMER_TIMER_WHEEL_H__

#include <uk/timer.h>
#include <uk/list.h>

void uktimer_wheel_add(struct uk_timer *timer);
void uktimer_wheel_tick(__u64 ticks);
__u64 uktimer_wheel_next_event(void);

#endif /* __UKTIMER_TIMER_WHEEL_H__ */
