#include <x86/traps.h>
#include <x86/ioapic.h>
#include <x86/apic.h>
#include <x86/acpi/madt.h>
#include <x86/cpu.h>
#include <kvm/pic.h>
#include <errno.h>
#include <stddef.h>

/* TODO Add support for multiple APIC*/

#define __IOAPIC_MAX_INTR 24
#define IRQ_VECTOR_BASE   32

volatile struct IOAPIC *ioapic;
static struct MADTType2Entry *interrupt_override[__IOAPIC_MAX_INTR] = { 0 };

#define IOAPIC_OVERRIDE_IRQ(irq) \
	(interrupt_override[irq] ? interrupt_override[irq]->GlobalSystemInterrupt \
							: irq)

void _wioapic_reg(__u8 reg, __u32 val)
{
	ioapic->ioregsel = reg;
	ioapic->iowin = val;
}

__u32 _rioapic_reg(__u8 reg)
{
	ioapic->ioregsel = reg;
	return ioapic->iowin;
}


void _wioapic_entry(__u8 index, __u64 entry)
{
	__u8 entry_reg;

	entry_reg = IOAPIC_REDTBL_ENTRY(index);
	_wioapic_reg(entry_reg, entry & 0xffffffff);
	_wioapic_reg(entry_reg + 1, entry >> 32);
}

__u64 _rioapic_entry(__u8 index)
{
	__u8 entry_reg;
	__u64 entry;

	entry_reg = IOAPIC_REDTBL_ENTRY(index);
	entry = _rioapic_reg(entry_reg);
	entry |= ((__u64) _rioapic_reg(entry_reg + 1)) << 32;

	return entry;
}


struct IOAPIC *detect_ioapic()
{
	struct MADT *madt;
	union MADTEntry m;
	struct IOAPIC *_ioapic = NULL;
	__sz off, len;

	madt = acpi_get_madt();
	len = madt->h.Length - sizeof(struct MADT);

	for (off = 0; off < len; off += m.h->Length) {
		m.h = (struct MADTEntryHeader *)(madt->Entries + off);
		switch(m.h->Type) {
			case MADT_TYPE_IO_APIC:
				/* For now we only support one APIC */
				if(_ioapic == NULL)
					_ioapic = (struct IOAPIC *)((__u64) m.e1->IOAPICAddress);
				break;
			case MADT_TYPE_INT_SRC_OVERRIDE:
				interrupt_override[m.e2->IRQSource] = m.e2;
				break;
			case MADT_TYPE_NMI_SOURCE:
				/* TODO parse NMI sources */
				break;
		}
	}
	return _ioapic;
}

/* write 01 to IMRC */
int ioapic_enable(void)
{
	extern volatile struct IOAPIC *ioapic;

	if((ioapic = detect_ioapic()) == NULL)
		return ENOENT;

	uk_pr_debug("IOAPIC enabled\n");
	/* enable APIC */
	outb(IMCR_ADDR, 0x70);
	outb(IMCR_DATA, 0x01);
	return 0;
}

int ioapic_init(void)
{
	uint8_t max_entries;
	unsigned int global_system_interrupt;
	union IOAPICVer ver;
	union REDTBLEntry e = INIT_REDTBL_ENTRY();

	ver.dword = _rioapic_reg(IOAPIC_VER_REG);
	if(ver.ioapic_version != IOAPIC_VER)
		return -1;

	max_entries = ver.max_red_entry + 1;

	//e.mask = 0;
	for (int irq_no = 0; irq_no < max_entries; irq_no++) {
		_wioapic_entry(irq_no, e.qword);
		e.qword = _rioapic_entry(irq_no);
		uk_pr_debug(">>>>>>>>>>>>>>>>>>>>>>>> ENTRY IS %d\n", e.vector);
		e.vector++;
	}

	for(int irq_no = 0; irq_no < max_entries; irq_no++) {
		if(!interrupt_override[irq_no])
			continue;
		global_system_interrupt = 
			interrupt_override[irq_no]->GlobalSystemInterrupt;
		e.vector = IRQ_VECTOR_BASE + irq_no;
		_wioapic_entry(global_system_interrupt, e.qword);
	}

	/* TODO: Configure NMIs and make them edge triggered */

	return 0;
}

void ioapic_mask_irq(unsigned int irq)
{
	union REDTBLEntry e;

	irq = IOAPIC_OVERRIDE_IRQ(irq);
	e.qword = _rioapic_entry(irq);
	e.mask = IOAPIC_INT_MASK_SET;
	_wioapic_entry(irq, e.qword);
}

void ioapic_clear_irq(unsigned int irq)
{
	union REDTBLEntry e;

	//irq = IOAPIC_OVERRIDE_IRQ(irq);
	irq = 2;
	e.qword = _rioapic_entry(irq);
	e.mask = IOAPIC_INT_MASK_UNSET;
	_wioapic_entry(irq, e.qword); \
}

uint32_t ioapic_get_max_irqs(void)
{
	union IOAPICVer ver;

	ver.dword = _rioapic_reg(IOAPIC_VER_REG);
	return (uint32_t)ver.max_red_entry + 1;
}

void ioapic_set_irq_affinity(unsigned int irq, __u8 affinity)
{
	union REDTBLEntry e;

	irq = IOAPIC_OVERRIDE_IRQ(irq);
	e.qword = _rioapic_entry(irq);
	e.destination_field = (uint8_t) affinity;
	_wioapic_entry(irq, e.qword);
}

void ioapic_set_trigger_type(unsigned int irq, __u8 trigger)
{
	union REDTBLEntry e;

	irq = IOAPIC_OVERRIDE_IRQ(irq);
	e.qword = _rioapic_entry(irq);
	e.trigger_mode = trigger;
	_wioapic_entry(irq, e.qword);
}

/* Dummy functions */
void ioapic_set_irq_prio(unsigned int irq, __u8 priority)
{
	return;
}

void ioapic_handle_irq(void)
{
	return;
}
