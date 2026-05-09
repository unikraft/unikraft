/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors. */

/*
 * Lightweight syscall profiler for the Hyperlight plat.
 *
 * Per-syscall-number (count, cumulative-ns) counters, populated via the
 * syscall_shim enter/exit tabs. Meant as a diagnostic knob for chasing
 * per-syscall cost gaps between native Linux and in-guest runs — not a
 * production profiler. The monotonic clock read on each enter/exit is
 * TSC-backed on Hyperlight so overhead per hook is a few ns; it still
 * shows up at the noise floor for very fast syscalls, so very short
 * calls (<200 ns native) look inflated here.
 *
 * Dump via hyperlight_syscall_profile_dump() — the dispatcher calls it
 * right before pushing a void result, so every host→guest call reports
 * its own snapshot to stderr.
 */

#include <uk/arch/types.h>
#include <uk/syscall.h>
#include <uk/syscall_entertab.h>
#include <uk/syscall_exittab.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/essentials.h>

/* Raw console byte emitter — bypasses uk_printk so we can dump from
 * contexts (e.g. right after a user-ELF callback returns) where
 * TLS/GS-dependent formatting code still points at user state and
 * faults. Writes go to the same debug port console.c uses.
 */
#define HL_PORT_DEBUG_PRINT 103

static inline void sp_outb(__u8 value)
{
	__asm__ __volatile__("outb %0, %1"
			     : : "a"(value),
			         "Nd"((__u16)HL_PORT_DEBUG_PRINT));
}

static void sp_puts(const char *s)
{
	for (; *s; s++)
		sp_outb((__u8)*s);
}

static void sp_putlu(unsigned long v)
{
	char buf[24];
	int i = 0;
	if (v == 0) {
		sp_outb('0');
		return;
	}
	while (v && i < (int)sizeof(buf)) {
		buf[i++] = '0' + (v % 10);
		v /= 10;
	}
	while (i > 0)
		sp_outb((__u8)buf[--i]);
}

static void sp_put_padded(const char *s, int width)
{
	int len = 0;
	while (s[len])
		len++;
	sp_puts(s);
	for (int i = len; i < width; i++)
		sp_outb(' ');
}

static void sp_put_lu_padded(unsigned long v, int width)
{
	char buf[24];
	int i = 0;
	if (v == 0) {
		buf[i++] = '0';
	} else {
		while (v && i < (int)sizeof(buf)) {
			buf[i++] = '0' + (v % 10);
			v /= 10;
		}
	}
	for (int pad = i; pad < width; pad++)
		sp_outb(' ');
	while (i > 0)
		sp_outb((__u8)buf[--i]);
}

#define SYSPROF_MAX 512

struct sysprof_entry {
	__u64 count;
	__u64 total_ns;
};

static struct sysprof_entry g_sysprof[SYSPROF_MAX];

/* Monotonic-clock reading at enter, kept in .bss as a simple per-thread
 * scratch. Hyperlight is single-threaded on the syscall hot path so a
 * plain global is enough — upgrading to TLS or per-lcpu is a follow-up
 * if/when the plat goes multi-threaded. */
static __u64 g_enter_ns;

static void sysprof_enter(struct uk_syscall_enter_ctx *enter_ctx)
{
	if (!(enter_ctx->flags & UK_SYSCALL_ENTER_CTX_BINARY_SYSCALL))
		return;

	/* If the user ELF is on the way out via exit/_exit/exit_group,
	 * the matching syscall exit-tab hook will never fire because the
	 * process terminates inside the syscall. Dump the counters NOW
	 * before they vanish.
	 */
	long n = _UK_EXECENV_REGS_GET(enter_ctx->execenv, __syscall_rsyscall);
	if (n == 60 /* exit */ || n == 231 /* exit_group */)
		hyperlight_syscall_profile_dump();

	g_enter_ns = ukplat_monotonic_clock();
}

static void sysprof_exit(struct uk_syscall_exit_ctx *exit_ctx)
{
	if (!(exit_ctx->flags & UK_SYSCALL_EXIT_CTX_BINARY_SYSCALL))
		return;
	__u64 now = ukplat_monotonic_clock();
	__u64 dt = (now > g_enter_ns) ? (now - g_enter_ns) : 0;
	long n = _UK_EXECENV_REGS_GET(exit_ctx->execenv, __syscall_rsyscall);
	if (n >= 0 && n < SYSPROF_MAX) {
		g_sysprof[n].count++;
		g_sysprof[n].total_ns += dt;
	}
}

/* NOTE: the entertab/exittab macros use `#prio` directly in the
 * section name. `UK_PRIO_EARLIEST` (a named macro) stringifies to the
 * *name* rather than its numeric value, which produces a section the
 * lds.S `[0-9]` glob doesn't pick up — so the hook silently doesn't
 * register. Pass the raw digit so the section name is
 * `.uk_syscall_entertab0` / `.uk_syscall_exittab9`, matching the lds.S.
 */
uk_syscall_entertab_prio(sysprof_enter, 0);
uk_syscall_exittab_prio(sysprof_exit, 9);

static void sysprof_sort_topn(int topn, int *out_indices)
{
	/* Tiny insertion sort of indices by total_ns descending. */
	int filled = 0;
	for (int i = 0; i < SYSPROF_MAX; i++) {
		if (g_sysprof[i].count == 0)
			continue;
		int insert = filled;
		while (insert > 0
		       && g_sysprof[out_indices[insert - 1]].total_ns
		          < g_sysprof[i].total_ns)
			insert--;
		if (insert >= topn)
			continue;
		int shift_to = (filled < topn) ? filled : topn - 1;
		for (int j = shift_to; j > insert; j--)
			out_indices[j] = out_indices[j - 1];
		out_indices[insert] = i;
		if (filled < topn)
			filled++;
	}
	for (int i = filled; i < topn; i++)
		out_indices[i] = -1;
}

/* Dump the per-syscall count + cumulative-ns counters direct to the
 * debug console port. Avoids uk_printk's TLS/GS-reliant format path
 * because this is called from dispatch_inner right after the user
 * ELF returned, when TLS registers still point at user state.
 * Sorted by total_ns descending so the biggest offenders land on top.
 */
void hyperlight_syscall_profile_dump(void)
{
	int topn = 24;
	int indices[24];
	sysprof_sort_topn(topn, indices);

	sp_puts("sysprof:  nr name                 count      total_us     avg_ns\n");
	__u64 sum_ns = 0;
	__u64 sum_count = 0;
	for (int i = 0; i < topn; i++) {
		int n = indices[i];
		if (n < 0)
			break;
		__u64 count = g_sysprof[n].count;
		__u64 total = g_sysprof[n].total_ns;
		__u64 avg = count ? (total / count) : 0;
		sp_puts("sysprof: ");
		sp_put_lu_padded((unsigned long)n, 3);
		sp_puts(" ");
		sp_put_padded(uk_syscall_name(n), 20);
		sp_put_lu_padded(count, 8);
		sp_puts("  ");
		sp_put_lu_padded(total / 1000, 12);
		sp_puts("  ");
		sp_put_lu_padded(avg, 10);
		sp_outb('\n');
		sum_ns += total;
		sum_count += count;
	}
	for (int i = 0; i < SYSPROF_MAX; i++) {
		int in_topn = 0;
		for (int j = 0; j < topn; j++)
			if (indices[j] == i) {
				in_topn = 1;
				break;
			}
		if (!in_topn && g_sysprof[i].count) {
			sum_ns += g_sysprof[i].total_ns;
			sum_count += g_sysprof[i].count;
		}
	}
	sp_puts("sysprof: total calls=");
	sp_putlu(sum_count);
	sp_puts(" total_us=");
	sp_putlu(sum_ns / 1000);
	sp_outb('\n');

	/* Finer-grained mmap breakdown: ukmmap exports these raw counters
	 * so we can see how much of the per-mmap budget is VMA bookkeeping
	 * vs. the per-page demand-map loop vs. pread's vfscore descent.
	 *
	 * Guarded by CONFIG_LIBUKMMAP because the counters themselves live
	 * in lib/ukmmap; an image that doesn't enable LIBUKMMAP (e.g.
	 * helloworld-c, which has no need for mmap) would otherwise fail
	 * to link with undefined references to mmap_timing_*.
	 */
#if CONFIG_LIBUKMMAP
	extern __u64 mmap_timing_calls;
	extern __u64 mmap_timing_bookkeep_ns;
	extern __u64 mmap_timing_pgloop_ns;
	extern __u64 mmap_timing_pread_ns;
	extern __u64 mmap_timing_pgloop_pages;
	extern __u64 mmap_timing_pread_bytes;

	sp_puts("mmap_time: calls=");
	sp_putlu(mmap_timing_calls);
	sp_puts(" bookkeep_us=");
	sp_putlu(mmap_timing_bookkeep_ns / 1000);
	sp_puts(" pgloop_us=");
	sp_putlu(mmap_timing_pgloop_ns / 1000);
	sp_puts(" pread_us=");
	sp_putlu(mmap_timing_pread_ns / 1000);
	sp_puts(" pgloop_pages=");
	sp_putlu(mmap_timing_pgloop_pages);
	sp_puts(" pread_bytes=");
	sp_putlu(mmap_timing_pread_bytes);
	sp_outb('\n');
#endif /* CONFIG_LIBUKMMAP */
}

/* Reset the counters — handy for separating evolve-time noise from the
 * call-phase work we actually want to measure. */
void hyperlight_syscall_profile_reset(void)
{
	for (int i = 0; i < SYSPROF_MAX; i++) {
		g_sysprof[i].count = 0;
		g_sysprof[i].total_ns = 0;
	}
}
