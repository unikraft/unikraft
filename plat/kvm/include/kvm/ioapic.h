#ifndef __PLAT_CMN_X86_IOAPIC_H
#define __PLAT_CMN_X86_IOAPIC_H
/* ACPI:
 *	MADT structure: type 1
 *	field			 byte_len byte_off Description
 *	Type			 1        0
 *	Length			 1        1        12
 *	I/O APIC ID		 1		  2		   The IOAPIC's ID
 *	Reserved		 1		  3        0
 *	I/O APIC address 4		  4		   32 bit physical address of the IOAPIC base
 */

#include <x86/redtbl.h>
#include <x86/apic.h>
#include <uk/essentials.h>
#include <uk/arch/types.h>
#include <stdint.h>

#define IOAPIC_VER					0x11

#define IOAPIC_ID_REG				0x00
#define IOAPIC_VER_REG				0x01
#define IOAPIC_ARB_REG				0x02
#define IOAPIC_REDTBL_BASE_REG		0x10

#define IOAPIC_REGSEL				0x00
#define IOAPIC_WIN					0x10

#define IOAPIC_REDTBL_ENTRY(index) (IOAPIC_REDTBL_BASE_REG + index * 2)

struct IOAPIC {
	__u32 ioregsel;
	__u32 __padding[3];
	__u32 iowin;
} __packed;

union IOAPICID {
	struct {
		__u32 dword;
	};
	struct {
		__u32 reserved1:24;
		__u32 ioapic_id:4;
		__u32 reserved2:4;
	};
} __packed;

union IOAPICVer {
	struct {
		__u32 dword;
	};
	struct {
		__u32 ioapic_version:8;
		__u32 reserved1:8;
		__u32 max_red_entry:8;
		__u32 reserved2:8;
	};
} __packed;

union IOAPICArb{
	struct {
		__u32 dword;
	};
	struct {
		__u32 reserved1:24;
		__u32 ioapic_arb_id:4;
		__u32 reseerved2:24;
	};
} __packed;

#define	IOAPIC_DELIVERY_MODE_FIXED				0
#define	IOAPIC_DELIVERY_MODE_LOWEST_PRIORITY	1
#define	IOAPIC_DELIVERY_MODE_SMI				2
#define	IOAPIC_DELIVERY_MODE_RESERVED0			3
#define	IOAPIC_DELIVERY_MODE_NMI				4
#define	IOAPIC_DELIVERY_MODE_INIT				5
#define	IOAPIC_DELIVERY_MODE_RESERVED1			6
#define	IOAPIC_DELIVERY_MODE_EXTINT				7


#define IOAPIC_DEST_MODE_PHYSICAL	0
#define IOAPIC_DEST_MODE_LOGICAL	1

#define IOAPIC_DELIVERY_STATUS_IDLE		0
#define IOAPIC_DELIVERY_STATUS_SENDING	0

#define IOAPIC_INT_POLARITY_ACT_HIGH	0
#define IOAPIC_INT_POLARITY_ACT_LOW	0

#define IOAPIC_REMOTE_IRR_ACCEPTED	1

#define IOAPIC_LEVEL_TRIGGER_MODE	1
#define IOAPIC_EDGE_TRIGGER_MODE	0

#define IOAPIC_INT_MASK_SET	1
#define IOAPIC_INT_MASK_UNSET	0


/* init the vectors to disabled state */
int ioapic_enable(void);
int ioapic_init(void);
void ioapic_mask_irq(unsigned int irq);
void ioapic_clear_irq(unsigned int irq);
void ioapic_set_trigger_type(unsigned int irq, __u8 trigger);
void ioapic_set_irq_affinity(unsigned int irq, __u8 affinity);
uint32_t ioapic_get_max_irqs(void);

/* Dummy functions */
void ioapic_set_irq_prio(unsigned int irq, uint8_t priority);
void ioapic_handle_irq(void);
#endif
