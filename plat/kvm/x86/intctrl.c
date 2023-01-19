#include <kvm/intctrl.h>
#include <kvm/pic.h>
#include <uk/print.h>
#include <x86/apic.h>
#include <kvm/ioapic.h>

struct _pic_dev *pic;

/* TODO: This will go to pic-common.c */
int _madt_init_pic(struct MADT *madt, struct _pic_dev **pic)
{
	int rc = 0;

#if CONFIG_HAVE_SMP
	rc = ioapic_probe(madt, pic);
	if (rc == 0) {
		disable_pic();
		return 0;
	}
#endif

	rc = pic_probe(madt, pic);
	if (rc == 0)
		return 0;


	return rc;
}

void intctrl_init(void)
{
	int rc;
	struct MADT *madt = NULL;

#if CONFIG_HAVE_SMP
	madt = acpi_get_madt();
	if(madt == NULL)
		goto EXIT_ERR;
#endif

	rc = _madt_init_pic(madt, &pic);
	if(unlikely(rc))
		goto EXIT_ERR;

	rc = pic->ops.initialize();
	if(unlikely(rc))
		goto EXIT_ERR;

	return;

EXIT_ERR:
	UK_CRASH("Failed to initialized PIC\n");
}

uint32_t intctrl_get_max_irqs(void)
{
	return pic->ops.get_max_irqs();
}

void intctrl_ack_irq(unsigned int irq)
{
	pic->ops.ack_irq(irq);
	return;
}

void intctrl_mask_irq(unsigned int irq)
{
	pic->ops.disable_irq(irq);
	return;
}

void intctrl_clear_irq(unsigned int irq)
{
	pic->ops.enable_irq(irq);
	return;
}
