#ifndef __PLAT_CMN_X86_LVT_H
#define __PLAT_CMN_X86_LVT_H

#define LVT_DELIVERY_MODE_FIXED		0
#define LVT_DELIVERY_MODE_SMI		2
#define LVT_DELIVERY_MODE_NMI		4
#define LVT_DELIVERY_MODE_INIT		5
#define LVT_DELIVERY_MODE_RESERVED	6
#define LVT_DELIVER_MODE_EXTINT		7

#define LVT_DELIVERY_STATUS_IDLE	0
#define LVT_DELIVERY_STATUS_SND_PND 1

#define LVT_INT_POLARITY_ACT_HIGH	0
#define LVT_INT_POLARITY_ACT_LOW	1

#define LVT_EDGE_TRIGGER_MODE	0
#define LVT_LEVEL_TRIGGER_MODE	1

#define LVT_INT_MASK_SET	1
#define LVT_INT_MASK_UNSET	0

#define LVT_TIMER_MODE_ONE_SHOT		0
#define LVT_TIMER_MODE_PERIODIC		1
#define LVT_TIMER_MODE_TSC_DEADLINE 2
#define LVT_TIMER_MODE_ONE_RESERVED 3

/* Local APIC local vector table entry */
union LVTEntry {
	struct {
		__u32 dword;
	};
	struct {
		__u32 vector:8; /* IRQ vector */
		__u32 delivery_mode:3; /* Type of interrupt */
		__u32 reserved1:1; /* Reserved */
		__u32 delivery_status:1; /* Indicates interrupt delivery status (r-only)*/
		__u32 polarity:1; /* Polarity of the interrupt pin */
		__u32 remote_irr:1; /* Interrupt accepted by LAPIC (r-only) */
		__u32 trigger_mode:1; /* Selects the trigger mode for LINT0 and LINT1 */
		__u32 mask:1; /* Interrupt mask */
		__u32 timer_mode:2; /* Timer mode */
		__u32 reserved2:13; /* Reserved */
	};
};

#define INITIALIZE_LVT_ENTRY() { \
	.vector = 0, \
	.delivery_mode = LVT_DELIVERY_MODE_FIXED, \
	.reserved1 = 0,	\
	.delivery_status = LVT_DELIVERY_STATUS_IDLE, \
	.polarity = LVT_INT_POLARITY_ACT_HIGH, \
	.remote_irr = 0, \
	.trigger_mode = LVT_EDGE_TRIGGER_MODE, \
	.mask = LVT_INT_MASK_SET, \
	.timer_mode = LVT_TIMER_MODE_ONE_SHOT, \
	.reserved2 = 0,	\
}

#ifdef CONFIG_HAVE_SMP 
	#define set_lvt_entry_attr(msr, attr, value) ({	\
			union LVTEntry __entry;	\
			__u32 edx = 0; \
			rdmsr(msr, &__entry.dword, &edx); \
			__entry.attr = value; \
			wrmsr(msr, __entry.dword, edx);	\
		})

	#define get_lvt_entry_attr(msr, attr, value) ({	\
			union LVTEntry __entry;	\
			__u32 edx; \
			rdmsr(msr, &__entry.dword, &edx); \
			__entry.attr; \
		})
#endif


#endif
