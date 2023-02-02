#ifndef __PLAT_KVM_X86_IOAPIC_H
#define __PLAT_KVM_X86_IOAPIC_H
/* ACPI:
 *	MADT structure: type 1
 *	field			 byte_len byte_off Description
 *	Type			 1        0
 *	Length			 1        1        12
 *	I/O APIC ID		 1		  2		   The IOAPIC's ID
 *	Reserved		 1		  3        0
 *	I/O APIC address 4		  4		   32 bit physical address of the IOAPIC base
 */

#include <kvm/redtbl.h>
#include <kvm/intctrl.h>
#include <x86/apic.h>
#include <x86/acpi/acpi.h>
#include <uk/essentials.h>
#include <uk/arch/types.h>
#include <stdint.h>


#define IOAPIC_VER					0x11 /* The version our driver supports */

#define IOAPIC_ID_REG				0x00
#define IOAPIC_VER_REG				0x01
#define IOAPIC_ARB_REG				0x02
#define IOAPIC_REDTBL_BASE_REG		0x10

#define IOAPIC_REGSEL				0x00
#define IOAPIC_WIN					0x10

#define IOAPIC_REDTBL_ENTRY(index) (IOAPIC_REDTBL_BASE_REG + index * 2)

/* This structure points to the memory mapped IOAPIC
 * The start of memory is found out by reading the ACPI MADT entry
 */
struct IOAPIC {
	__u32 ioregsel;
	__u32 __padding[3];
	__u32 iowin;
} __packed;

/* IOAPIC identification register 
 *
 * 	This register contains the physical IOAPIC id which is used during the bus
 * 	arbitration
 *
 * 	In multiple IOAPIC environment each IOAPIC must programmed with unique
 *
 * 	IOAPIC during initialization
 */
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

/* IOAPIC version register 
 */
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

/* IOAPIC arbitration register
 *
 * 	This register contains the arbitration priority of the IOAPIC 
 *
 * 	The APIC uses the rotating priority to win bus ownership
 * 	
 * 	This is initially loaded with the IOAPIC id
 */
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
#define IOAPIC_INT_POLARITY_ACT_LOW		0

#define IOAPIC_REMOTE_IRR_ACCEPTED	1

#define IOAPIC_LEVEL_TRIGGER_MODE	1
#define IOAPIC_EDGE_TRIGGER_MODE	0

#define IOAPIC_INT_MASK_SET	1
#define IOAPIC_INT_MASK_UNSET	0

/* Probe the IOAPIC using ACPI MADT 
 * 
 * @param madt
 *  The ACPI madt structure
 *
 * @param dev
 *  The interrupt controller device, which is to be initialized
 */
int ioapic_probe(const struct MADT *madt, struct _pic_dev **dev);
#endif
