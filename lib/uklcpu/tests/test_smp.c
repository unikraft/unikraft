/* SPDX-License-Identifier: BSD-3-Clause */
/* Test-kernel-only logical CPU startup and non-yielding overlap probe. */
#include <errno.h>
#include <uk/arch/ctx.h>
#include <uk/arch/x86_64.h>
#include <uk/asm.h>
#include <uk/lcpu.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/test.h>

#define PROBE_STACK_BYTES (64 * 1024)
#define PROBE_TIMEOUT_NS ((__nsec)2000000000ULL)

struct cpu_probe_slot {
	unsigned int entered;
	__u64 hardware_id;
	unsigned long progress;
};

static struct cpu_probe_slot slots[CONFIG_UKPLAT_CPU_MAXCOUNT];
static unsigned char stacks[CONFIG_UKPLAT_CPU_MAXCOUNT][PROBE_STACK_BYTES]
	__align(16);
static unsigned int release_probe;
static int callback_error;

static __u64 observed_hardware_id(void)
{
	__u32 a, b, c, d, maximum;

	uk_arch_x86_64_cpuid(0, 0, &maximum, &b, &c, &d);
	if (maximum >= 0xb) {
		uk_arch_x86_64_cpuid(0xb, 0, &a, &b, &c, &d);
		if (b)
			return d;
	}
	uk_arch_x86_64_cpuid(1, 0, &a, &b, &c, &d);
	return b >> 24;
}

static void remote_probe(struct uk_lcpu_regs *regs __unused, void *arg __unused)
{
	const __u64 index = uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx);

	if (!index || index >= CONFIG_UKPLAT_CPU_MAXCOUNT) {
		__atomic_store_n(&callback_error, -EINVAL, __ATOMIC_RELEASE);
		return;
	}
	slots[index].hardware_id = observed_hardware_id();
	__atomic_store_n(&slots[index].progress, 1, __ATOMIC_RELAXED);
	__atomic_store_n(&slots[index].entered, 1, __ATOMIC_RELEASE);
	/* No scheduler, allocation, I/O, TLS or voluntary yield on remote CPUs. */
	while (!__atomic_load_n(&release_probe, __ATOMIC_ACQUIRE)) {
		__atomic_fetch_add(&slots[index].progress, 1, __ATOMIC_RELAXED);
		uk_arch_spinwait();
	}
}

UK_TESTCASE(lcpu_smp, bootstrap_and_overlap)
{
	__u64 indices[CONFIG_UKPLAT_CPU_MAXCOUNT - 1];
	__u64 stack_pointers[CONFIG_UKPLAT_CPU_MAXCOUNT - 1];
	const unsigned int cpu_count = uk_lcpu_discovered_count();
	const unsigned int remote_count = cpu_count - 1;
	__u64 expected_mask = 0;
	const struct uk_lcpu_func function = { .fn = remote_probe, .user = NULL };
	unsigned int started = remote_count, ready, queued = 0, participants = 0;
	__u64 mask = 0, progress_mask = 0;
	__nsec deadline;
	int rc = 0, wait_rc;

	if (!cpu_count || cpu_count > CONFIG_UKPLAT_CPU_MAXCOUNT || cpu_count > 64) {
		rc = -EINVAL;
		goto finish;
	}
	expected_mask = ~0ULL >> (64 - cpu_count);
	if (uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx) != 0) {
		rc = -EINVAL;
		goto finish;
	}
	for (unsigned int i = 1; i < cpu_count; i++) {
		if (uk_pcpuvar_lval(i, uk_pcpuvar_cpu_idx) != i) {
			rc = -ENODEV;
			goto finish;
		}
		indices[i - 1] = i;
		stack_pointers[i - 1] =
			(__u64)ukarch_gen_sp(stacks[i], sizeof(stacks[i]));
	}
	slots[0].hardware_id = observed_hardware_id();
	slots[0].progress = 1;
	slots[0].entered = 1;
	if (!remote_count) {
		mask = 1;
		goto finish;
	}
	rc = uk_lcpu_start(indices, &started, stack_pointers, NULL, 0);
	if (rc || started != remote_count) {
		if (!rc)
			rc = -EIO;
		goto finish;
	}
	ready = remote_count;
	rc = uk_lcpu_wait(indices, &ready, PROBE_TIMEOUT_NS);
	if (rc || ready != remote_count) {
		if (!rc)
			rc = -ETIMEDOUT;
		goto finish;
	}
	queued = remote_count;
	rc = uk_lcpu_run(indices, &queued, &function, UK_LCPU_RFLG_DONOTBLOCK);
	if (rc || queued != remote_count) {
		if (!rc)
			rc = -EIO;
		goto finish;
	}
	deadline = ukplat_monotonic_clock() + PROBE_TIMEOUT_NS;
	for (;;) {
		mask = 0;
		for (unsigned int i = 0; i < cpu_count; i++)
			if (__atomic_load_n(&slots[i].entered, __ATOMIC_ACQUIRE))
				mask |= 1ULL << i;
		if (mask == expected_mask)
			break;
		rc = __atomic_load_n(&callback_error, __ATOMIC_ACQUIRE);
		if (rc)
			break;
		if (ukplat_monotonic_clock() >= deadline) {
			rc = -ETIMEDOUT;
			break;
		}
		slots[0].progress++;
		uk_arch_spinwait();
	}
finish:
	/* Release all callbacks even when startup, dispatch or polling failed. */
	__atomic_store_n(&release_probe, 1, __ATOMIC_RELEASE);
	if (queued) {
		ready = queued;
		wait_rc = uk_lcpu_wait(indices, &ready, PROBE_TIMEOUT_NS);
		if (!rc && (wait_rc || ready != queued))
			rc = wait_rc ? wait_rc : -ETIMEDOUT;
	}
	if (!rc) {
		for (unsigned int i = 0; i < cpu_count; i++) {
			if (slots[i].hardware_id !=
			    uk_pcpuvar_lval(i, uk_pcpuvar_cpu_id))
				rc = -EIO;
			for (unsigned int j = 0; j < i; j++)
				if (slots[i].hardware_id == slots[j].hardware_id)
					rc = -EIO;
			if (__atomic_load_n(&slots[i].progress, __ATOMIC_ACQUIRE))
				progress_mask |= 1ULL << i;
			participants +=
				__atomic_load_n(&slots[i].entered, __ATOMIC_ACQUIRE) != 0;
		}
		if (mask != expected_mask || progress_mask != expected_mask)
			rc = -EIO;
	}
	UK_TEST_EXPECT_ZERO(rc);
	if (!rc) {
		uk_pr_info("SMP_PROBE discovered_count=%u\n", cpu_count);
		for (unsigned int i = 0; i < cpu_count; i++)
			uk_pr_info("SMP_PROBE cpu=%u hw=%llu progress=%lu\n", i,
				   (unsigned long long)slots[i].hardware_id,
				   slots[i].progress);
		uk_pr_info("SMP_PROBE overlap=%u participants=%u mask=%llx progress_mask=%llx\n",
			   (unsigned int)(cpu_count > 1), participants,
			   (unsigned long long)mask,
			   (unsigned long long)progress_mask);
	}
	/* CPU stacks remain live until this isolated test guest shuts down. */
}

uk_testsuite_late_prio(lcpu_smp, NULL, 8);
