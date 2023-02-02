#include <kvm/intctrl.h>
#include <kvm/pic.h>
#include <kvm/ioapic.h>
#include <x86/apic.h>
#include <uk/plat/common/irq.h>
#include <uk/print.h>

struct _pic_dev *pic;

/* TODO: This will go to pic-common.c */
int _madt_init_pic(struct MADT *madt, struct _pic_dev **pic)
{
	int rc = 0;

#if CONFIG_HAVE_SMP
	/* Try to probe the IOAPIC first */
	rc = ioapic_probe(madt, pic);
	if (rc == 0) {
		uk_pr_info("IOAPIC found\n");
		disable_pic();
		return 0;
	}
#endif

	/* If IOAPIC not found probe legacy PIC */
	rc = pic_probe(madt, pic);
	if (rc == 0) {
		uk_pr_info("PIC found\n");
		return 0;
	}

	return rc;
}

void intctrl_init(void)
{
	int rc;
	struct MADT *madt = NULL;

#if CONFIG_HAVE_SMP
	madt = acpi_get_madt();
	if (madt == NULL)
		goto EXIT_ERR;
#endif

	rc = _madt_init_pic(madt, &pic);
	if (unlikely(rc))
		goto EXIT_ERR;

	/* Initialize PIC */
	rc = pic->ops.initialize();
	if (unlikely(rc))
		goto EXIT_ERR;

	return;

EXIT_ERR:
	UK_CRASH("Failed to initialized PIC\n");
}

uint32_t intctrl_get_max_irqs(void)
{
	uint32_t irq = 0;
	if (pic->ops.get_max_irqs)
		irq = pic->ops.get_max_irqs();
	return irq;
}

void intctrl_ack_irq(uint32_t irq)
{
	if (pic->ops.ack_irq)
		pic->ops.ack_irq(irq);
	return;
}

void intctrl_mask_irq(uint32_t irq)
{
	if (pic->ops.disable_irq)
		pic->ops.disable_irq(irq);
	return;
}

void intctrl_clear_irq(uint32_t irq)
{
	if (pic->ops.enable_irq)
		pic->ops.enable_irq(irq);
	return;
}

void intctrl_set_irq_type(uint32_t irq, enum uk_irq_trigger trigger)
{
	if(pic->ops.set_irq_type)
		pic->ops.set_irq_type(irq, trigger);
	return;
}

void intctrl_set_irq_affinity(uint32_t irq, uint8_t affinity)
{
	if(pic->ops.set_irq_affinity)
		pic->ops.set_irq_affinity(irq, affinity);
	return;
}
