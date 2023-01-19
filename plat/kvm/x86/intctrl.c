#include <kvm/intctrl.h>
#include <kvm/pic.h>
#include <uk/print.h>
#include <x86/apic.h>
#include <x86/ioapic.h>

struct IntCtrlOps pic_int_ctrl_ops = {
	.initialize = pic_init,
	.ack_irq = pic_ack_irq,
	.enable_irq = pic_clear_irq,
	.disable_irq = pic_mask_irq,
	.set_irq_type = pic_set_trigger_type,
	.set_irq_prio = pic_set_irq_prio,
	.set_irq_affinity = pic_set_irq_affinity,
	.handle_irq = pic_handle_irq,
	.get_max_irqs = pic_get_max_irqs,
};

struct IntCtrlOps ioapic_int_ctrl_ops = {
	.initialize = ioapic_init,
	.ack_irq = apic_ack_interrupt,
	.enable_irq = ioapic_clear_irq,
	.disable_irq = ioapic_mask_irq,
	.set_irq_type = ioapic_set_trigger_type,
	.set_irq_prio = ioapic_set_irq_prio,
	.set_irq_affinity = ioapic_set_irq_affinity,
	.handle_irq = ioapic_handle_irq,
	.get_max_irqs = ioapic_get_max_irqs,
};

struct IntCtrl ic = {
	.is_ioapic_present = 0,
	.is_initialized = 0,
	.intctrl_ops = NULL,
};

void intctrl_init(void)
{
	ic.intctrl_ops = &pic_int_ctrl_ops;
	if(ioapic_enable()) {
		ic.is_ioapic_present = 0;
		ic.intctrl_ops = &pic_int_ctrl_ops;
	}
	else {
		uk_pr_debug("PIC disabled\n");
		disable_pic();
		ic.is_ioapic_present = 1;
		ic.intctrl_ops = &ioapic_int_ctrl_ops;
	}
	ic.intctrl_ops->initialize();
	ic.is_initialized = 1;
}

uint32_t intctrl_get_max_irqs(void)
{
	return ic.intctrl_ops->get_max_irqs();
}

void intctrl_ack_irq(unsigned int irq)
{
	ic.intctrl_ops->ack_irq(irq);
}

void intctrl_mask_irq(unsigned int irq)
{
	ic.intctrl_ops->disable_irq(irq);
}

void intctrl_clear_irq(unsigned int irq)
{
	uk_pr_debug("Clearing IRQ %d\n", irq);
	ic.intctrl_ops->enable_irq(irq);
}
