/* SPDX-License-Identifier: BSD-3-Clause */
/* Test-kernel-only logical CPU startup and non-yielding overlap probe. */
#include <errno.h>
#include <uk/alloc.h>
#include <uk/arch/tls.h>
#include <uk/arch/ctx.h>
#include <uk/arch/x86_64.h>
#include <uk/asm.h>
#include <uk/lcpu.h>
#include <uk/paging.h>
#include <uk/vmem.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/test.h>

#define PROBE_STACK_BYTES (64 * 1024)
#define PROBE_TIMEOUT_NS ((__nsec)2000000000ULL)

struct cpu_probe_slot {
	unsigned int entered;
	__u64 hardware_id;
	unsigned long progress;
	void *tls_area;
	__uptr expected_tlsp;
	__uptr expected_auxsp;
	__uptr observed_tlsp;
	__uptr observed_auxsp;
	__paddr_t observed_page_table_root;
	__u64 observed_efer;
#if CONFIG_HAVE_SYSCALL
	__u64 observed_star;
	__u64 observed_lstar;
	__u64 observed_fmask;
	__u64 observed_gs_base;
	__u64 observed_kernel_gs_base;
#endif
	unsigned long tls_marker;
	unsigned int context_ready;
	unsigned int startup_stack_checked;
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
	unsigned int boot_stage;
#endif
};

static struct cpu_probe_slot slots[CONFIG_UKPLAT_CPU_MAXCOUNT];
static unsigned char stacks[CONFIG_UKPLAT_CPU_MAXCOUNT][PROBE_STACK_BYTES]
	__align(16);
static unsigned char auxstacks[CONFIG_UKPLAT_CPU_MAXCOUNT][PROBE_STACK_BYTES]
	__align(UKARCH_AUXSP_ALIGN);
static __thread unsigned long probe_tls_marker;
static struct uk_vas *probe_vas;
static unsigned int release_probe;
static int callback_error;

#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
static void probe_stage(__u64 index, unsigned int stage)
{
	__atomic_store_n(&slots[index].boot_stage, stage, __ATOMIC_RELEASE);
}
#else
#define probe_stage(index, stage) do { } while (0)
#endif

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

/* Kept out of the TLS-register switching function so TLS address calculations
 * cannot be reused across the switch. No remote allocator or scheduler yet.
 */
static __noinline int record_context(__u64 index, unsigned long marker)
{
	const __uptr tlsp = uk_lcpu_tlsp_get();
	const __paddr_t root = uk_pal_pt_read_base();
	const __u64 efer = uk_arch_x86_64_rdmsrl(UK_ARCH_X86_64_MSR_EFER);
	const __uptr auxsp = uk_pcpuvar_current_get(UK_LCPU_AUXSP_SYM);
	struct ukarch_auxspcb *cb;

#if CONFIG_HAVE_SYSCALL
	{
		const __u64 star = uk_arch_x86_64_rdmsrl(UK_ARCH_X86_64_MSR_STAR);
		const __u64 lstar = uk_arch_x86_64_rdmsrl(UK_ARCH_X86_64_MSR_LSTAR);
		const __u64 fmask =
			uk_arch_x86_64_rdmsrl(UK_ARCH_X86_64_MSR_SYSCALL_MASK);
		const __u64 gs = uk_arch_x86_64_rdmsrl(UK_ARCH_X86_64_MSR_GS_BASE);
		const __u64 kgs =
			uk_arch_x86_64_rdmsrl(UK_ARCH_X86_64_MSR_KERNEL_GS_BASE);

		if (!(efer & UK_ARCH_X86_64_EFER_SCE) || !star || !lstar ||
		    !fmask || gs != kgs || (index ? !gs : gs != 0))
			return -EIO;
		slots[index].observed_star = star;
		slots[index].observed_lstar = lstar;
		slots[index].observed_fmask = fmask;
		slots[index].observed_gs_base = gs;
		slots[index].observed_kernel_gs_base = kgs;
	}
#endif
	if (!(efer & UK_ARCH_X86_64_EFER_NXE) ||
	    !probe_vas || uk_vas_get_active() != probe_vas ||
	    uk_paging_pt_get_active() != probe_vas->pt ||
	    root != probe_vas->pt->pt_pbase ||
	    !tlsp || !auxsp || tlsp != slots[index].expected_tlsp ||
	    auxsp != slots[index].expected_auxsp)
		return -EIO;
	cb = ukarch_auxsp_get_cb(auxsp);
	if (ukarch_auxspcb_get_uktlsp(cb) != tlsp || probe_tls_marker != marker)
		return -EIO;
	slots[index].observed_page_table_root = root;
	slots[index].observed_efer = efer;
	slots[index].observed_tlsp = tlsp;
	slots[index].observed_auxsp = auxsp;
	slots[index].tls_marker = probe_tls_marker;
	return 0;
}

static __noinline int publish_secondary_context(__u64 index)
{
	const __uptr sp = uk_arch_read_sp();
	int rc;

	if (sp < (__uptr)stacks[index] ||
	    sp >= (__uptr)stacks[index] + sizeof(stacks[index]) ||
	    probe_tls_marker != 0)
		return -EIO;
	probe_tls_marker = index + 1;
	rc = record_context(index, index + 1);
	if (rc)
		return rc;
	slots[index].startup_stack_checked = 1;
	__atomic_store_n(&slots[index].context_ready, 1, __ATOMIC_RELEASE);
	return 0;
}

static void __noreturn secondary_probe_entry(struct uk_lcpu *this_lcpu)
{
	const __u64 index = uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx);
	int rc;

	/* Custom platform entries own CPU initialization, unlike the default
	 * entry used by the preceding hardware-only probe.
	 */
	if (!index || index >= CONFIG_UKPLAT_CPU_MAXCOUNT) {
		__atomic_store_n(&callback_error, -EINVAL, __ATOMIC_RELEASE);
		uk_lcpu_halt();
	}
	probe_stage(index, 1);
	rc = uk_lcpu_init(this_lcpu);
	if (rc)
		goto failed;
	probe_stage(index, 2);
	/* The trampoline still uses its boot page tables. Activate the
	 * captured runtime address space before touching allocated TLS.
	 */
	rc = uk_vas_set_active(probe_vas);
	if (rc)
		goto failed;
	probe_stage(index, 3);
	uk_lcpu_tlsp_set(slots[index].expected_tlsp);
	probe_stage(index, 4);
	uk_lcpu_init_auxsp(slots[index].expected_auxsp,
			   slots[index].expected_tlsp);
	probe_stage(index, 5);
	rc = publish_secondary_context(index);
	if (rc)
		goto failed;
	probe_stage(index, 6);
	uk_lcpu_idle();
failed:
	__atomic_store_n(&callback_error, rc, __ATOMIC_RELEASE);
	uk_lcpu_halt();
}

static void remote_probe(struct uk_lcpu_regs *regs __unused, void *arg __unused)
{
	const __u64 index = uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx);

	if (!index || index >= CONFIG_UKPLAT_CPU_MAXCOUNT) {
		__atomic_store_n(&callback_error, -EINVAL, __ATOMIC_RELEASE);
		return;
	}
	if (!__atomic_load_n(&slots[index].context_ready, __ATOMIC_ACQUIRE) ||
	    record_context(index, index + 1)) {
		__atomic_store_n(&callback_error, -EIO, __ATOMIC_RELEASE);
		return;
	}
	probe_tls_marker = CONFIG_UKPLAT_CPU_MAXCOUNT + index + 1;
	if (record_context(index, probe_tls_marker)) {
		__atomic_store_n(&callback_error, -EIO, __ATOMIC_RELEASE);
		return;
	}
	slots[index].hardware_id = observed_hardware_id();
	__atomic_store_n(&slots[index].progress, 1, __ATOMIC_RELAXED);
	__atomic_store_n(&slots[index].entered, 1, __ATOMIC_RELEASE);
	/* Own TLS and auxiliary context; no remote allocator, scheduler, I/O
	 * or voluntary yield in this isolated startup probe.
	 */
	while (!__atomic_load_n(&release_probe, __ATOMIC_ACQUIRE)) {
		__atomic_fetch_add(&slots[index].progress, 1, __ATOMIC_RELAXED);
		uk_arch_spinwait();
	}
}

UK_TESTCASE(lcpu_smp, bootstrap_and_overlap)
{
	__u64 indices[CONFIG_UKPLAT_CPU_MAXCOUNT - 1];
	__u64 stack_pointers[CONFIG_UKPLAT_CPU_MAXCOUNT - 1];
	__uptr entries[CONFIG_UKPLAT_CPU_MAXCOUNT - 1];
	struct uk_alloc *a = uk_alloc_get_default();
	const unsigned int cpu_count = uk_lcpu_discovered_count();
	const unsigned int remote_count = cpu_count - 1;
	__u64 expected_mask = 0;
	const struct uk_lcpu_func function = { .fn = remote_probe, .user = NULL };
	unsigned int started = remote_count, ready, queued = 0, participants = 0;
	__u64 mask = 0, progress_mask = 0;
	__nsec deadline;
	int rc = 0, wait_rc;
	unsigned int wait_phase = 0;

	if (!cpu_count || cpu_count > CONFIG_UKPLAT_CPU_MAXCOUNT || cpu_count > 64) {
		rc = -EINVAL;
		goto finish;
	}
	expected_mask = ~0ULL >> (64 - cpu_count);
	if (uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx) != 0) {
		rc = -EINVAL;
		goto finish;
	}
	probe_vas = uk_vas_get_active();
	if (!probe_vas || !probe_vas->pt) {
		rc = -EINVAL;
		goto finish;
	}
	if (!a) {
		rc = -ENOMEM;
		goto finish;
	}
	for (unsigned int i = 1; i < cpu_count; i++) {
		if (uk_pcpuvar_lval(i, uk_pcpuvar_cpu_idx) != i) {
			rc = -ENODEV;
			goto finish;
		}
		slots[i].tls_area = uk_memalign(a, ukarch_tls_area_align(),
					   ukarch_tls_area_size());
		if (!slots[i].tls_area) {
			rc = -ENOMEM;
			goto finish;
		}
		ukarch_tls_area_init(slots[i].tls_area);
		slots[i].expected_tlsp = ukarch_tls_tlsp(slots[i].tls_area);
		slots[i].expected_auxsp =
			ukarch_gen_sp(auxstacks[i], sizeof(auxstacks[i]));
		entries[i - 1] = (__uptr)secondary_probe_entry;
		indices[i - 1] = i;
		stack_pointers[i - 1] =
			(__u64)ukarch_gen_sp(stacks[i], sizeof(stacks[i]));
	}
	slots[0].expected_tlsp = uk_lcpu_tlsp_get();
	slots[0].expected_auxsp = uk_pcpuvar_current_get(UK_LCPU_AUXSP_SYM);
	probe_tls_marker = 1;
	rc = record_context(0, 1);
	if (rc)
		goto finish;
	slots[0].context_ready = 1;
	slots[0].hardware_id = observed_hardware_id();
	slots[0].progress = 1;
	slots[0].entered = 1;
	if (!remote_count) {
		mask = 1;
		goto finish;
	}
	wait_phase = 1;
	rc = uk_lcpu_start(indices, &started, stack_pointers, entries, 0);
	if (rc || started != remote_count) {
		if (!rc)
			rc = -EIO;
		goto finish;
	}
	ready = remote_count;
	wait_phase = 2;
	rc = uk_lcpu_wait(indices, &ready, PROBE_TIMEOUT_NS);
	if (rc || ready != remote_count) {
		if (!rc)
			rc = -ETIMEDOUT;
		goto finish;
	}
	queued = remote_count;
	wait_phase = 3;
	rc = uk_lcpu_run(indices, &queued, &function, UK_LCPU_RFLG_DONOTBLOCK);
	if (rc || queued != remote_count) {
		if (!rc)
			rc = -EIO;
		goto finish;
	}
	wait_phase = 4;
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
	if (!rc)
		rc = __atomic_load_n(&callback_error, __ATOMIC_ACQUIRE);
	if (!rc)
		rc = record_context(0, 1);
	if (!rc) {
		for (unsigned int i = 0; i < cpu_count; i++) {
			if (!__atomic_load_n(&slots[i].context_ready, __ATOMIC_ACQUIRE) ||
			    (i && !slots[i].startup_stack_checked) ||
			    slots[i].tls_marker !=
				(i ? CONFIG_UKPLAT_CPU_MAXCOUNT + i + 1 : 1))
				rc = -EIO;
			for (unsigned int j = 0; j < i; j++)
				if (slots[i].observed_tlsp == slots[j].observed_tlsp ||
				    slots[i].observed_auxsp == slots[j].observed_auxsp)
					rc = -EIO;
#if CONFIG_HAVE_SYSCALL
			if (slots[i].observed_star != slots[0].observed_star ||
			    slots[i].observed_lstar != slots[0].observed_lstar ||
			    slots[i].observed_fmask != slots[0].observed_fmask ||
			    slots[i].observed_gs_base !=
				slots[i].observed_kernel_gs_base)
				rc = -EIO;
			for (unsigned int j = 0; j < i; j++)
				if (slots[i].observed_gs_base ==
				    slots[j].observed_gs_base)
					rc = -EIO;
#endif
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
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
	if (rc) {
		uk_pr_err("SMP_BOOT_FAILURE phase=%u rc=%d started=%u queued=%u callback_error=%d\n",
			  wait_phase, rc, started, queued,
			  __atomic_load_n(&callback_error, __ATOMIC_ACQUIRE));
		for (unsigned int i = 1;
		     i < cpu_count && i < CONFIG_UKPLAT_CPU_MAXCOUNT; i++)
			uk_pr_err("SMP_BOOT_STATE cpu=%u stage=%u state=%d error=%d ready=%u\n",
				  i, __atomic_load_n(&slots[i].boot_stage, __ATOMIC_ACQUIRE),
				  __atomic_load_n(&uk_pcpuvar_lval(i, uk_lcpus).state, __ATOMIC_ACQUIRE),
				  uk_pcpuvar_lval(i, uk_lcpus).error_code,
				  __atomic_load_n(&slots[i].context_ready, __ATOMIC_ACQUIRE));
	}
#endif
	UK_TEST_EXPECT_ZERO(rc);
	if (!rc) {
		uk_pr_info("SMP_PROBE discovered_count=%u\n", cpu_count);
		for (unsigned int i = 0; i < cpu_count; i++) {
			uk_pr_info("SMP_ADDRESS cpu=%u root=%lx efer=%llx\n", i,
				   (unsigned long)slots[i].observed_page_table_root,
				   (unsigned long long)slots[i].observed_efer);
#if CONFIG_HAVE_SYSCALL
			uk_pr_info("SMP_SYSCALL cpu=%u sce=%u star=%llx lstar=%llx fmask=%llx gs=%llx kgs=%llx\n",
				   i,
				   (unsigned int)!!(slots[i].observed_efer &
						    UK_ARCH_X86_64_EFER_SCE),
				   (unsigned long long)slots[i].observed_star,
				   (unsigned long long)slots[i].observed_lstar,
				   (unsigned long long)slots[i].observed_fmask,
				   (unsigned long long)slots[i].observed_gs_base,
				   (unsigned long long)slots[i].observed_kernel_gs_base);
#endif
			uk_pr_info("SMP_CONTEXT cpu=%u tls=%lx aux=%lx marker=%lu ready=%u startup_stack_checked=%u\n",
				   i, (unsigned long)slots[i].observed_tlsp,
				   (unsigned long)slots[i].observed_auxsp,
				   slots[i].tls_marker, slots[i].context_ready,
				   slots[i].startup_stack_checked);
			uk_pr_info("SMP_PROBE cpu=%u hw=%llu progress=%lu\n", i,
				   (unsigned long long)slots[i].hardware_id,
				   slots[i].progress);
		}
		uk_pr_info("SMP_PROBE overlap=%u participants=%u mask=%llx progress_mask=%llx\n",
			   (unsigned int)(cpu_count > 1), participants,
			   (unsigned long long)mask,
			   (unsigned long long)progress_mask);
	}
	/* CPU stacks, TLS allocations and auxiliary stacks remain live until
	 * this isolated test guest shuts down, including partial startup failure.
	 */
}

uk_testsuite_late_prio(lcpu_smp, NULL, 8);
