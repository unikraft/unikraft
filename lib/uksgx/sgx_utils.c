#include <uk/sgx_internal.h>
#include <uk/sgx_asm.h>
#include <uk/sgx_cpu.h>
#include <uk/config.h>
#include <uk/print.h>
#include <kvm-x86/traps.h>

typedef int(cpl_switch_handler_t)(__u8 rpl);

cpl_switch_handler_t *cpl_switch_handler;

/*
 * Actually unikernel does not need ring-3, however,
 * ENCLU instruction in only avaliable in ring-3. Hence,
 * we need somehow to switch the CPL from ring-0 to ring-3
 * when the CPU is running enclave code.
 */

int legacy_cpl_switch(__u8 rpl)
{
	asm volatile(""
		     "mov %0, %%rax;"
		     "mov %%rax, %%ds;"
		     "mov %%rax, %%es;"
		     "mov %%rsp, %%rax;"
		     "push %0;"
		     "push %%rax;"
		     "pushf;"
		     "push %1;"
		     "push $1f;"
		     "iretq;"
		     "1:" ::"g"(GDT_DESC_OFFSET(GDT_DESC_USER_DATA) | 3),
		     "g"(GDT_DESC_OFFSET(GDT_DESC_USER_CODE) | 3));
}

int fast_cpl_switch(__u8 rpl)
{
	static __u8 is_lstar_set = 0;
	if (!is_lstar_set) {
		is_lstar_set = 1;
		wrmsrl_safe(X86_MSR_IA32_LSTAR, (__u64) && success);
	}
	if (rpl == 0) { /* syscall */
		/* We do not need to save rsp as the stack will not be changed
		 */
		asm volatile("syscall;");
	} else { /* rpl == 3, sysret */
		asm volatile(""
			     "movq %0, %%rcx;"
			     "pushfq;"
			     "popq %%r11;"
			     "orq $0x200, %%r11;"
			     "sysretq;"
			     "1:" ::"g"(&&success));
	}
success:
	return;
}

int cpl_switch(__u8 rpl)
{
	__u8 cpl;

	if (rpl != 0 && rpl != 3) {
		goto err_invalid_rpl;
	}

	cpl = get_cpl();
	if (cpl == rpl) {
		goto success;
	}

	if (cpl_switch_handler == NULL) {
		goto err_not_init;
	}

	return cpl_switch_handler(rpl);

success:
	return 0;

err_invalid_rpl:
	uk_pr_err("Invalid rpl %d\n", rpl);
	return -EINVAL;

err_not_init:
	uk_pr_err("CPL switch handler is not initialized\n");
	return -EINIT;
}

void cpl_switch_init()
{
	unsigned int info[4] = {0, 0, 0, 0};
	unsigned int *eax, *ebx, *ecx, *edx;
	unsigned long ret;
	__u8 cpl;

	if (cpl_switch_handler != NULL) {
		return;
	}

	eax = &info[0];
	ebx = &info[1];
	ecx = &info[2];
	edx = &info[3];

	// check if extended feature is enabled
	__cpuid(info, 0x80000001);
	if (!(*edx & (0x1 << 20)) && !(*edx & (0x1 << 29))) {
		uk_pr_info("Extended feature is not enabled\n");
		cpl_switch_handler = &legacy_cpl_switch;
	}

	// enable syscall/sysret (IA32_EFER.[0] = 1)])
	ret = rdmsrl(X86_MSR_IA32_EFER);

	if ((ret & X86_EFER_SYSCALL) == 0) {
		uk_pr_info("syscall/sysret instruction is not enabled, trying "
			   "to enable them\n");
		ret |= X86_EFER_SYSCALL;
		wrmsrl_safe(X86_MSR_IA32_EFER, ret);
		ret = rdmsrl(X86_MSR_IA32_EFER);
		if (!(ret & X86_EFER_SYSCALL)) {
			uk_pr_info(
			    "syscall/sysret instruction enabled unsucessfully, "
			    "fallback to legacy switch\n");
			cpl_switch_handler = &legacy_cpl_switch;
		} else {
			cpl_switch_handler = &fast_cpl_switch;

			/*
			 * SYSRET hardcodes selectors and registers:
			 *
			 *   if returning to 32-bit userspace:
			 *      cs = IA32_LSTAR[63:48] | 3,
			 *   if returning to 64-bit userspace:
			 *      cs = IA32_LSTAR[63:48]+16 | 3,
			 *
			 * ss = IA32_LSTAR[63:48]+8 (in either case)
			 * rip = rcx
			 *
			 * as Unikraft works only under 64-bit mode, we should
			 * set STAR.SYSRET_CS to 0x10 ((0x10 + 16) | 3 = 0x23 is
			 * the userspace cs selector). hence ss = (0x10 + 8) | 3
			 * = 0x1b
			 *
			 * SYSCALL hardcodes selectors and registers:
			 *
			 *    cs = IA32_STAR[47:32] & 0xFFFC (force RPL to 0)
			 *    ss = IA32_STAR[47:32] + 8
			 *    rip = IA32_LSTAR
			 *
			 * IA32_STAR[63:48] should be set to 0x10,
			 * IA32_STAR[47:32] is the kernel CS selector 0x8.
			 */
			wrmsr_safe(X86_MSR_IA32_STAR, 0,
				   GDT_DESC_OFFSET(GDT_DESC_DATA) << 16
				       | GDT_DESC_OFFSET(GDT_DESC_CODE));
			/*
			 * wrmsrl_safe(X86_MSR_IA32_LSTAR, (__u64) && success);
			 *
			 * We cannot initialize LSTAR here, as the target
			 * address is not known yet. However, we can initialize
			 * it before the first call of fast_cpl_switch() because
			 * it must be called from ring-0 at the first time.
			 */
		}
	}
}
