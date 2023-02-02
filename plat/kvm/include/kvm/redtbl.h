#ifndef __PLAT_CMN_X86_REDTBL_H
#define __PLAT_CMN_X86_REDTBL_H

#include <uk/arch/types.h>

/* I/O APIC redirection table entry */
union REDTBLEntry {
	struct {
		__u64 qword;
	};
	struct {
		__u64 vector:8; /* The IRQ vector */
		__u64 delivery_mode:3; /* The type of the interrupt  */
		__u64 destination_mode:1; /* Determines if dest is logical or physical */
		__u64 delivery_status:1; /* Indicates the Interrupt delivery status (r-only) */
		__u64 polarity:1; /* Polarity of the interrupt pin */
		__u64 remote_irr:1; /* Interrupt accepted by the LAPIC */
		__u64 trigger_mode:1; /* Selects the trigger mode */
		__u64 mask:1; /* Interrupt mask */
		__u64 reserved:39; /* Reserved */
		__u64 destination_field:8; /* Destination processor of the interrupt */
	};
} __packed;

#define INIT_REDTBL_ENTRY() { \
	.vector = 0, \
	.delivery_mode = IOAPIC_DELIVERY_MODE_FIXED, \
	.destination_mode = IOAPIC_DEST_MODE_PHYSICAL, \
	.delivery_status = IOAPIC_DELIVERY_STATUS_IDLE, \
	.polarity = IOAPIC_INT_POLARITY_ACT_HIGH, \
	.remote_irr = 0, \
	.trigger_mode = IOAPIC_EDGE_TRIGGER_MODE, \
	.mask = IOAPIC_INT_MASK_SET, \
	.reserved = 0, \
	.destination_field = 0,	\
}
#endif
