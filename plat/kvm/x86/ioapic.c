#include <x86/traps.h>
#include <x86/apic.h>
#include <x86/acpi/madt.h>
#include <x86/cpu.h>
#include <kvm/ioapic.h>
#include <kvm/pic.h>
#include <kvm/intctrl.h>
#include <uk/plat/common/irq.h>
#include <errno.h>
#include <stddef.h>

/* TODO Add support for multiple APIC*/

#define __IOAPIC_MAX_INTR 24
#define IRQ_VECTOR_BASE   32

static volatile struct IOAPIC *ioapic;

/* Interrupt Source Overrides are necessary to describe variances between 
 * the IA-PC standard dual 8259 interrupt definition and the platform’s
 * implementation.
 * For example, if your machine has the ISA Programmable Interrupt Timer (PIT)
 * connected to ISA IRQ 0, but in APIC mode, it is connected to I/O APIC 
 * interrupt input 2, then you would need an Interrupt Source Override where 
 * the source entry is ‘0’ and the Global System Interrupt is ‘2.’
 */
static struct MADTType2Entry *interrupt_override[__IOAPIC_MAX_INTR] = { 0 };

#define IOAPIC_OVERRIDE_IRQ(irq) \
	(interrupt_override[irq] ? \
	 interrupt_override[irq]->GlobalSystemInterrupt \
	 : irq)

int ioapic_enable(void);
int ioapic_init(void);
void ioapic_mask_irq(uint32_t irq);
void ioapic_clear_irq(uint32_t irq);
void ioapic_set_trigger_type(uint32_t irq, enum uk_irq_trigger trigger);
void ioapic_set_irq_affinity(uint32_t irq, uint8_t affinity);
uint32_t ioapic_get_max_irqs(void);
void ioapic_set_irq_prio(uint32_t irq, uint8_t priority);
void ioapic_handle_irq(void);

static inline void _wioapic_reg(uint8_t reg, __u32 val)
{
	ioapic->ioregsel = reg;
	ioapic->iowin = val;
}

static inline uint32_t _rioapic_reg(uint8_t reg)
{
	ioapic->ioregsel = reg;
	return ioapic->iowin;
}


static inline void _wioapic_entry(uint8_t index, __u64 entry)
{
	uint8_t entry_reg;

	entry_reg = IOAPIC_REDTBL_ENTRY(index);
	_wioapic_reg(entry_reg, entry & 0xffffffff);
	_wioapic_reg(entry_reg + 1, entry >> 32);
}

static inline __u64 _rioapic_entry(uint8_t index)
{
	__u8 entry_reg;
	__u64 low;
	__u64 high;

	entry_reg = IOAPIC_REDTBL_ENTRY(index);
	low = _rioapic_reg(entry_reg);
	high = _rioapic_reg(entry_reg + 1);

	return low | (high << 32);
}


struct _pic_dev ioapic_drv = {
	.is_probed = 0,
	.is_initialized = 0,
};

static int ioapic_do_probe(const struct MADT *madt)
{
	union MADTEntry m;
	__sz off, len;

	struct _pic_operations ioapic_ops = {
		.initialize			= ioapic_init,
		.ack_irq			= apic_ack_interrupt,
		.enable_irq			= ioapic_clear_irq,
		.disable_irq		= ioapic_mask_irq,
		.set_irq_type		= ioapic_set_trigger_type,
		.set_irq_affinity	= ioapic_set_irq_affinity,
		.get_max_irqs		= ioapic_get_max_irqs,
	};

	len = madt->h.Length - sizeof(struct MADT);

	/* Search the MADT entry for IOAPIC */
	for (off = 0; off < len; off += m.h->Length) {

		m.h = (struct MADTEntryHeader *)(madt->Entries + off);

		switch (m.h->Type) {
		case MADT_TYPE_IO_APIC:
			/* TODO: Add support for multiple IRQ */
			if (ioapic == NULL)
				ioapic = (struct IOAPIC *)((__u64) m.e1->IOAPICAddress);
			break;
		case MADT_TYPE_INT_SRC_OVERRIDE:
			interrupt_override[m.e2->IRQSource] = m.e2;
			break;
		case MADT_TYPE_NMI_SOURCE:
			/* TODO parse NMI sources */
			break;
		}
	}

	if (ioapic == NULL)
		return -ENODEV; /* IOAPIC not found */

	ioapic_drv.ops = ioapic_ops;
	ioapic_drv.is_probed = 1;

	return 0;
}

int ioapic_probe(const struct MADT *madt, struct _pic_dev **dev)
{
	int rc = 0;

	if (ioapic_drv.is_probed) {
		*dev = &ioapic_drv;
		return 0;
	}

	rc = ioapic_do_probe(madt);
	if (unlikely(rc))
		return rc;

	/* Switch from the virtual-wire/PIC mode to symmetric mode 
	 */
	outb(IMCR_ADDR, 0x70);
	outb(IMCR_DATA, 0x01);

	*dev = &ioapic_drv;
	return 0;
}


int ioapic_init(void)
{
	uint8_t max_entries;
	uint32_t global_system_interrupt;
	union IOAPICVer ver;
	union REDTBLEntry e = INIT_REDTBL_ENTRY();

	ver.dword = _rioapic_reg(IOAPIC_VER_REG);
	if (ver.ioapic_version != IOAPIC_VER)
		return -ENODEV;

	max_entries = ver.max_red_entry + 1;

	/* Intialize the redirection table entries (32 onwards) */
	for (int irq_no = 0; irq_no < max_entries; irq_no++) {
		//e.destination_field = 1;
		e.vector = IRQ_VECTOR_BASE + irq_no;
		_wioapic_entry(irq_no, e.qword);
	}

	/* Reassign the Interrupt vector based on interrupt override structure */
	for (int irq_no = 0; irq_no < max_entries; irq_no++) {
		if (!interrupt_override[irq_no])
			continue;
		global_system_interrupt =
			interrupt_override[irq_no]->GlobalSystemInterrupt;
		e.vector = IRQ_VECTOR_BASE + irq_no;
		_wioapic_entry(global_system_interrupt, e.qword);
	}

	/* TODO: Configure NMIs and make them edge triggered */

	return 0;
}

void ioapic_mask_irq(uint32_t irq)
{
	union REDTBLEntry e;

	irq = IOAPIC_OVERRIDE_IRQ(irq);
	e.qword = _rioapic_entry(irq);
	e.mask = IOAPIC_INT_MASK_SET;
	_wioapic_entry(irq, e.qword);
}

void ioapic_clear_irq(uint32_t irq)
{
	union REDTBLEntry e;

	irq = IOAPIC_OVERRIDE_IRQ(irq);
	e.qword = _rioapic_entry(irq);
	e.mask = IOAPIC_INT_MASK_UNSET;
	_wioapic_entry(irq, e.qword);
}

uint32_t ioapic_get_max_irqs(void)
{
	union IOAPICVer ver;

	ver.dword = _rioapic_reg(IOAPIC_VER_REG);
	return (uint32_t)ver.max_red_entry + 1;
}


void ioapic_set_irq_affinity(uint32_t irq, uint8_t affinity)
{
	union REDTBLEntry e;

	irq = IOAPIC_OVERRIDE_IRQ(irq);
	e.qword = _rioapic_entry(irq);
	e.destination_field = (uint8_t) affinity;
	_wioapic_entry(irq, e.qword);
}

void ioapic_set_trigger_type(uint32_t irq, enum uk_irq_trigger trigger)
{
	union REDTBLEntry e;
	uint8_t tgr;

	if (trigger == UK_IRQ_TRIGGER_NONE)
		return;
	else if (UK_IRQ_TRIGGER_EDGE)
		tgr = IOAPIC_EDGE_TRIGGER_MODE;
	else
		tgr = IOAPIC_LEVEL_TRIGGER_MODE;

	irq = IOAPIC_OVERRIDE_IRQ(irq);
	e.qword = _rioapic_entry(irq);
	e.trigger_mode = tgr;
	_wioapic_entry(irq, e.qword);
}

/* Dummy functions */
void ioapic_set_irq_prio(uint32_t irq __unused, uint8_t priority __unused)
{
	return;
}

void ioapic_handle_irq(void)
{
	return;
}
