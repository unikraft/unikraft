/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdio.h>

#include <uk/atomic.h>
#include <uk/console.h>
#include <uk/console/driver.h>
#include <uk/crash.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/semaphore.h>

#define LINES_PER_WRITER 16U

static struct uk_console *serial;
static struct uk_semaphore finished;
static unsigned int online;
static unsigned int arrived;
static unsigned long long cpu_mask;

static void write_direct(const char *text, int size)
{
	if (size < 0 || uk_console_out_direct(serial, text, (__sz)size) != size)
		UK_CRASH("Failed to write complete console test line\n");
}

static __noreturn void console_writer(void *unused __unused)
{
	unsigned int cpu = (unsigned int)uk_sched_current()->lcpu_idx;
	__nsec deadline = ukplat_monotonic_clock() + 3000000000ULL;
	char line[128];

	uk_or(&cpu_mask, 1ULL << cpu);
	uk_inc(&arrived);
	while (uk_load_n(&arrived) != online) {
		if (ukplat_monotonic_clock() >= deadline)
			UK_CRASH("Console writers did not run simultaneously\n");
		uk_arch_spinwait();
	}

	for (unsigned int i = 0; i < LINES_PER_WRITER; ++i) {
		unsigned long long value = ((unsigned long long)(cpu + 1) << 40) | i;
		int size = snprintf(line, sizeof(line),
			"CONSOLE_SMP_D cpu=%u seq=%u data=%016llx:%016llx\n", cpu, i, value, ~value);

		if (size < 0 || (__sz)size >= sizeof(line))
			UK_CRASH("Console test line exceeds its buffer\n");
		write_direct(line, size);
		uk_pr_info("CONSOLE_SMP_K cpu=%u seq=%u data=%016llx:%016llx\n", cpu, i, value, ~value);
		if (printf("CONSOLE_SMP_P cpu=%u seq=%u data=%016llx:%016llx\n",
			   cpu, i, value, ~value) != size)
			UK_CRASH("Failed to write complete stdio test line\n");
	}
	uk_semaphore_up(&finished);
	uk_sched_thread_exit();
}

/* Start after all application schedulers are available. */
static void __constructor run_console_smp(void)
{
	static const char begin[] = "CONSOLE_SMP_BEGIN\n";
	unsigned long long expected_mask = 0;
	char end[96];
	int size;

	serial = uk_console_get(0);
	online = uk_sched_started_count();
	if (!serial || serial->dclass != UK_CONSOLE_CLASS_UART || !online || online > 64)
		UK_CRASH("Missing console or unsupported CPU count\n");
	uk_semaphore_init(&finished, 0);
	write_direct(begin, sizeof(begin) - 1);
	for (unsigned int i = 0; i < online; ++i) {
		struct uk_sched *sched = uk_sched_select_round_robin(uk_sched_current());

		if (!sched || sched->lcpu_idx < 0 || sched->lcpu_idx >= 64 ||
		    (expected_mask & (1ULL << sched->lcpu_idx)))
			UK_CRASH("Console writers require one scheduler per CPU\n");
		expected_mask |= 1ULL << sched->lcpu_idx;
		if (!uk_sched_thread_create(sched, console_writer, NULL, "console-writer"))
			UK_CRASH("Failed to create console writer\n");
	}
	for (unsigned int i = 0; i < online; ++i)
		if (uk_semaphore_down_to(&finished, 10000000000ULL) == __NSEC_MAX)
			UK_CRASH("Console writer failed to complete\n");
	size = snprintf(end, sizeof(end), "CONSOLE_SMP_END mask=%llx lines=%u\n",
			  uk_load_n(&cpu_mask), online * LINES_PER_WRITER * 3U);
	write_direct(end, size);
}
