#include <uk/event.h>
#include <uk/pcpuvar.h>

#include <uk/plat/native/arch/except.h>

__uk_pcpuvar __u8 uk_plat_native_except_switch_stack;
__uk_pcpuvar __uptr uk_plat_native_except_stack_base;

UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_DEBUG);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_INVALID_OP);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_PAGE_FAULT);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_BUS_ERROR);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_MATH);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_SECURITY);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_SYSCALL);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_IRQ);
UK_EVENT(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED);

__isr __uptr uk_plat_native_except_get_except_stack_base(void)
{
	return uk_pcpuvar_current_get(uk_plat_native_except_stack_base);
}

__isr int uk_plat_native_except_init(void)
{
	uk_pcpuvar_current_set(uk_plat_native_except_switch_stack, 0);
	uk_pcpuvar_current_set(uk_plat_native_except_stack_base,
			       uk_plat_native_riscv64_traps_get_except_stack_base());
	uk_plat_native_riscv64_traps_init();
	return 0;
}
