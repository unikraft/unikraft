/* 
 * Structure definitions for HVM state that is held by Xen and must
 * be saved along with the domain's memory and device-model state.
 * 
 * Copyright (c) 2007 XenSource Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef __XEN_PUBLIC_HVM_SAVE_X86_H__
#define __XEN_PUBLIC_HVM_SAVE_X86_H__

/* 
 * Save/restore header: general info about the save file. 
 */

#define HVM_FILE_MAGIC   0x54381286
#define HVM_FILE_VERSION 0x00000001

struct hvm_save_header {
    __u32 magic;             /* Must be HVM_FILE_MAGIC */
    __u32 version;           /* File format version */
    __u64 changeset;         /* Version of Xen that saved this file */
    __u32 cpuid;             /* CPUID[0x01][%eax] on the saving machine */
    __u32 gtsc_khz;        /* Guest's TSC frequency in kHz */
};

DECLARE_HVM_SAVE_TYPE(HEADER, 1, struct hvm_save_header);


/*
 * Processor
 *
 * Compat:
 *     - Pre-3.4 didn't have msr_tsc_aux
 *     - Pre-4.7 didn't have fpu_initialised
 */

struct hvm_hw_cpu {
    __u8  fpu_regs[512];

    __u64 rax;
    __u64 rbx;
    __u64 rcx;
    __u64 rdx;
    __u64 rbp;
    __u64 rsi;
    __u64 rdi;
    __u64 rsp;
    __u64 r8;
    __u64 r9;
    __u64 r10;
    __u64 r11;
    __u64 r12;
    __u64 r13;
    __u64 r14;
    __u64 r15;

    __u64 rip;
    __u64 rflags;

    __u64 cr0;
    __u64 cr2;
    __u64 cr3;
    __u64 cr4;

    __u64 dr0;
    __u64 dr1;
    __u64 dr2;
    __u64 dr3;
    __u64 dr6;
    __u64 dr7;    

    __u32 cs_sel;
    __u32 ds_sel;
    __u32 es_sel;
    __u32 fs_sel;
    __u32 gs_sel;
    __u32 ss_sel;
    __u32 tr_sel;
    __u32 ldtr_sel;

    __u32 cs_limit;
    __u32 ds_limit;
    __u32 es_limit;
    __u32 fs_limit;
    __u32 gs_limit;
    __u32 ss_limit;
    __u32 tr_limit;
    __u32 ldtr_limit;
    __u32 idtr_limit;
    __u32 gdtr_limit;

    __u64 cs_base;
    __u64 ds_base;
    __u64 es_base;
    __u64 fs_base;
    __u64 gs_base;
    __u64 ss_base;
    __u64 tr_base;
    __u64 ldtr_base;
    __u64 idtr_base;
    __u64 gdtr_base;

    __u32 cs_arbytes;
    __u32 ds_arbytes;
    __u32 es_arbytes;
    __u32 fs_arbytes;
    __u32 gs_arbytes;
    __u32 ss_arbytes;
    __u32 tr_arbytes;
    __u32 ldtr_arbytes;

    __u64 sysenter_cs;
    __u64 sysenter_esp;
    __u64 sysenter_eip;

    /* msr for em64t */
    __u64 shadow_gs;

    /* msr content saved/restored. */
    __u64 msr_flags; /* Obsolete, ignored. */
    __u64 msr_lstar;
    __u64 msr_star;
    __u64 msr_cstar;
    __u64 msr_syscall_mask;
    __u64 msr_efer;
    __u64 msr_tsc_aux;

    /* guest's idea of what rdtsc() would return */
    __u64 tsc;

    /* pending event, if any */
    union {
        __u32 pending_event;
        struct {
            __u8  pending_vector:8;
            __u8  pending_type:3;
            __u8  pending_error_valid:1;
            __u32 pending_reserved:19;
            __u8  pending_valid:1;
        };
    };
    /* error code for pending event */
    __u32 error_code;

#define _XEN_X86_FPU_INITIALISED        0
#define XEN_X86_FPU_INITIALISED         (1U<<_XEN_X86_FPU_INITIALISED)
    __u32 flags;
    __u32 pad0;
};

struct hvm_hw_cpu_compat {
    __u8  fpu_regs[512];

    __u64 rax;
    __u64 rbx;
    __u64 rcx;
    __u64 rdx;
    __u64 rbp;
    __u64 rsi;
    __u64 rdi;
    __u64 rsp;
    __u64 r8;
    __u64 r9;
    __u64 r10;
    __u64 r11;
    __u64 r12;
    __u64 r13;
    __u64 r14;
    __u64 r15;

    __u64 rip;
    __u64 rflags;

    __u64 cr0;
    __u64 cr2;
    __u64 cr3;
    __u64 cr4;

    __u64 dr0;
    __u64 dr1;
    __u64 dr2;
    __u64 dr3;
    __u64 dr6;
    __u64 dr7;    

    __u32 cs_sel;
    __u32 ds_sel;
    __u32 es_sel;
    __u32 fs_sel;
    __u32 gs_sel;
    __u32 ss_sel;
    __u32 tr_sel;
    __u32 ldtr_sel;

    __u32 cs_limit;
    __u32 ds_limit;
    __u32 es_limit;
    __u32 fs_limit;
    __u32 gs_limit;
    __u32 ss_limit;
    __u32 tr_limit;
    __u32 ldtr_limit;
    __u32 idtr_limit;
    __u32 gdtr_limit;

    __u64 cs_base;
    __u64 ds_base;
    __u64 es_base;
    __u64 fs_base;
    __u64 gs_base;
    __u64 ss_base;
    __u64 tr_base;
    __u64 ldtr_base;
    __u64 idtr_base;
    __u64 gdtr_base;

    __u32 cs_arbytes;
    __u32 ds_arbytes;
    __u32 es_arbytes;
    __u32 fs_arbytes;
    __u32 gs_arbytes;
    __u32 ss_arbytes;
    __u32 tr_arbytes;
    __u32 ldtr_arbytes;

    __u64 sysenter_cs;
    __u64 sysenter_esp;
    __u64 sysenter_eip;

    /* msr for em64t */
    __u64 shadow_gs;

    /* msr content saved/restored. */
    __u64 msr_flags; /* Obsolete, ignored. */
    __u64 msr_lstar;
    __u64 msr_star;
    __u64 msr_cstar;
    __u64 msr_syscall_mask;
    __u64 msr_efer;
    /*__u64 msr_tsc_aux; COMPAT */

    /* guest's idea of what rdtsc() would return */
    __u64 tsc;

    /* pending event, if any */
    union {
        __u32 pending_event;
        struct {
            __u8  pending_vector:8;
            __u8  pending_type:3;
            __u8  pending_error_valid:1;
            __u32 pending_reserved:19;
            __u8  pending_valid:1;
        };
    };
    /* error code for pending event */
    __u32 error_code;
};

static inline int _hvm_hw_fix_cpu(void *h, __u32 size) {

    union hvm_hw_cpu_union {
        struct hvm_hw_cpu nat;
        struct hvm_hw_cpu_compat cmp;
    } *ucpu = (union hvm_hw_cpu_union *)h;

    if ( size == sizeof(struct hvm_hw_cpu_compat) )
    {
        /*
         * If we copy from the end backwards, we should
         * be able to do the modification in-place.
         */
        ucpu->nat.error_code = ucpu->cmp.error_code;
        ucpu->nat.pending_event = ucpu->cmp.pending_event;
        ucpu->nat.tsc = ucpu->cmp.tsc;
        ucpu->nat.msr_tsc_aux = 0;
    }
    /* Mimic the old behaviour by unconditionally setting fpu_initialised. */
    ucpu->nat.flags = XEN_X86_FPU_INITIALISED;

    return 0;
}

DECLARE_HVM_SAVE_TYPE_COMPAT(CPU, 2, struct hvm_hw_cpu, \
                             struct hvm_hw_cpu_compat, _hvm_hw_fix_cpu);

/*
 * PIC
 */

struct hvm_hw_vpic {
    /* IR line bitmasks. */
    __u8 irr;
    __u8 imr;
    __u8 isr;

    /* Line IRx maps to IRQ irq_base+x */
    __u8 irq_base;

    /*
     * Where are we in ICW2-4 initialisation (0 means no init in progress)?
     * Bits 0-1 (=x): Next write at A=1 sets ICW(x+1).
     * Bit 2: ICW1.IC4  (1 == ICW4 included in init sequence)
     * Bit 3: ICW1.SNGL (0 == ICW3 included in init sequence)
     */
    __u8 init_state:4;

    /* IR line with highest priority. */
    __u8 priority_add:4;

    /* Reads from A=0 obtain ISR or IRR? */
    __u8 readsel_isr:1;

    /* Reads perform a polling read? */
    __u8 poll:1;

    /* Automatically clear IRQs from the ISR during INTA? */
    __u8 auto_eoi:1;

    /* Automatically rotate IRQ priorities during AEOI? */
    __u8 rotate_on_auto_eoi:1;

    /* Exclude slave inputs when considering in-service IRQs? */
    __u8 special_fully_nested_mode:1;

    /* Special mask mode excludes masked IRs from AEOI and priority checks. */
    __u8 special_mask_mode:1;

    /* Is this a master PIC or slave PIC? (NB. This is not programmable.) */
    __u8 is_master:1;

    /* Edge/trigger selection. */
    __u8 elcr;

    /* Virtual INT output. */
    __u8 int_output;
};

DECLARE_HVM_SAVE_TYPE(PIC, 3, struct hvm_hw_vpic);


/*
 * IO-APIC
 */

union vioapic_redir_entry
{
    __u64 bits;
    struct {
        __u8 vector;
        __u8 delivery_mode:3;
        __u8 dest_mode:1;
        __u8 delivery_status:1;
        __u8 polarity:1;
        __u8 remote_irr:1;
        __u8 trig_mode:1;
        __u8 mask:1;
        __u8 reserve:7;
        __u8 reserved[4];
        __u8 dest_id;
    } fields;
};

#define VIOAPIC_NUM_PINS  48 /* 16 ISA IRQs, 32 non-legacy PCI IRQS. */

#define XEN_HVM_VIOAPIC(name, cnt)                      \
    struct name {                                       \
        __u64 base_address;                          \
        __u32 ioregsel;                              \
        __u32 id;                                    \
        union vioapic_redir_entry redirtbl[cnt];        \
    }

XEN_HVM_VIOAPIC(hvm_hw_vioapic, VIOAPIC_NUM_PINS);

#ifndef __XEN__
#undef XEN_HVM_VIOAPIC
#else
#undef VIOAPIC_NUM_PINS
#endif

DECLARE_HVM_SAVE_TYPE(IOAPIC, 4, struct hvm_hw_vioapic);


/*
 * LAPIC
 */

struct hvm_hw_lapic {
    __u64             apic_base_msr;
    __u32             disabled; /* VLAPIC_xx_DISABLED */
    __u32             timer_divisor;
    __u64             tdt_msr;
};

DECLARE_HVM_SAVE_TYPE(LAPIC, 5, struct hvm_hw_lapic);

struct hvm_hw_lapic_regs {
    __u8 data[1024];
};

DECLARE_HVM_SAVE_TYPE(LAPIC_REGS, 6, struct hvm_hw_lapic_regs);


/*
 * IRQs
 */

struct hvm_hw_pci_irqs {
    /*
     * Virtual interrupt wires for a single PCI bus.
     * Indexed by: device*4 + INTx#.
     */
    union {
        unsigned long i[16 / sizeof (unsigned long)]; /* DECLARE_BITMAP(i, 32*4); */
        __u64 pad[2];
    };
};

DECLARE_HVM_SAVE_TYPE(PCI_IRQ, 7, struct hvm_hw_pci_irqs);

struct hvm_hw_isa_irqs {
    /*
     * Virtual interrupt wires for ISA devices.
     * Indexed by ISA IRQ (assumes no ISA-device IRQ sharing).
     */
    union {
        unsigned long i[1];  /* DECLARE_BITMAP(i, 16); */
        __u64 pad[1];
    };
};

DECLARE_HVM_SAVE_TYPE(ISA_IRQ, 8, struct hvm_hw_isa_irqs);

struct hvm_hw_pci_link {
    /*
     * PCI-ISA interrupt router.
     * Each PCI <device:INTx#> is 'wire-ORed' into one of four links using
     * the traditional 'barber's pole' mapping ((device + INTx#) & 3).
     * The router provides a programmable mapping from each link to a GSI.
     */
    __u8 route[4];
    __u8 pad0[4];
};

DECLARE_HVM_SAVE_TYPE(PCI_LINK, 9, struct hvm_hw_pci_link);

/* 
 *  PIT
 */

struct hvm_hw_pit {
    struct hvm_hw_pit_channel {
        __u32 count; /* can be 65536 */
        __u16 latched_count;
        __u8 count_latched;
        __u8 status_latched;
        __u8 status;
        __u8 read_state;
        __u8 write_state;
        __u8 write_latch;
        __u8 rw_mode;
        __u8 mode;
        __u8 bcd; /* not supported */
        __u8 gate; /* timer start */
    } channels[3];  /* 3 x 16 bytes */
    __u32 speaker_data_on;
    __u32 pad0;
};

DECLARE_HVM_SAVE_TYPE(PIT, 10, struct hvm_hw_pit);


/* 
 * RTC
 */ 

#define RTC_CMOS_SIZE 14
struct hvm_hw_rtc {
    /* CMOS bytes */
    __u8 cmos_data[RTC_CMOS_SIZE];
    /* Index register for 2-part operations */
    __u8 cmos_index;
    __u8 pad0;
};

DECLARE_HVM_SAVE_TYPE(RTC, 11, struct hvm_hw_rtc);


/*
 * HPET
 */

#define HPET_TIMER_NUM     3    /* 3 timers supported now */
struct hvm_hw_hpet {
    /* Memory-mapped, software visible registers */
    __u64 capability;        /* capabilities */
    __u64 res0;              /* reserved */
    __u64 config;            /* configuration */
    __u64 res1;              /* reserved */
    __u64 isr;               /* interrupt status reg */
    __u64 res2[25];          /* reserved */
    __u64 mc64;              /* main counter */
    __u64 res3;              /* reserved */
    struct {                    /* timers */
        __u64 config;        /* configuration/cap */
        __u64 cmp;           /* comparator */
        __u64 fsb;           /* FSB route, not supported now */
        __u64 res4;          /* reserved */
    } timers[HPET_TIMER_NUM];
    __u64 res5[4*(24-HPET_TIMER_NUM)];  /* reserved, up to 0x3ff */

    /* Hidden register state */
    __u64 period[HPET_TIMER_NUM]; /* Last value written to comparator */
};

DECLARE_HVM_SAVE_TYPE(HPET, 12, struct hvm_hw_hpet);


/*
 * PM timer
 */

struct hvm_hw_pmtimer {
    __u32 tmr_val;   /* PM_TMR_BLK.TMR_VAL: 32bit free-running counter */
    __u16 pm1a_sts;  /* PM1a_EVT_BLK.PM1a_STS: status register */
    __u16 pm1a_en;   /* PM1a_EVT_BLK.PM1a_EN: enable register */
};

DECLARE_HVM_SAVE_TYPE(PMTIMER, 13, struct hvm_hw_pmtimer);

/*
 * MTRR MSRs
 */

struct hvm_hw_mtrr {
#define MTRR_VCNT 8
#define NUM_FIXED_MSR 11
    __u64 msr_pat_cr;
    /* mtrr physbase & physmask msr pair*/
    __u64 msr_mtrr_var[MTRR_VCNT*2];
    __u64 msr_mtrr_fixed[NUM_FIXED_MSR];
    __u64 msr_mtrr_cap;
    __u64 msr_mtrr_def_type;
};

DECLARE_HVM_SAVE_TYPE(MTRR, 14, struct hvm_hw_mtrr);

/*
 * The save area of XSAVE/XRSTOR.
 */

struct hvm_hw_cpu_xsave {
    __u64 xfeature_mask;        /* Ignored */
    __u64 xcr0;                 /* Updated by XSETBV */
    __u64 xcr0_accum;           /* Updated by XSETBV */
    struct {
        struct { char x[512]; } fpu_sse;

        struct hvm_hw_cpu_xsave_hdr {
            __u64 xstate_bv;         /* Updated by XRSTOR */
            __u64 xcomp_bv;          /* Updated by XRSTOR{C,S} */
            __u64 reserved[6];
        } xsave_hdr;                    /* The 64-byte header */
    } save_area;
};

#define CPU_XSAVE_CODE  16

/*
 * Viridian hypervisor context.
 */

struct hvm_viridian_domain_context {
    __u64 hypercall_gpa;
    __u64 guest_os_id;
    __u64 time_ref_count;
    __u64 reference_tsc;
};

DECLARE_HVM_SAVE_TYPE(VIRIDIAN_DOMAIN, 15, struct hvm_viridian_domain_context);

struct hvm_viridian_vcpu_context {
    __u64 vp_assist_msr;
    __u8  vp_assist_vector;
    __u8  _pad[7];
};

DECLARE_HVM_SAVE_TYPE(VIRIDIAN_VCPU, 17, struct hvm_viridian_vcpu_context);

struct hvm_vmce_vcpu {
    __u64 caps;
    __u64 mci_ctl2_bank0;
    __u64 mci_ctl2_bank1;
    __u64 mcg_ext_ctl;
};

DECLARE_HVM_SAVE_TYPE(VMCE_VCPU, 18, struct hvm_vmce_vcpu);

struct hvm_tsc_adjust {
    __u64 tsc_adjust;
};

DECLARE_HVM_SAVE_TYPE(TSC_ADJUST, 19, struct hvm_tsc_adjust);


struct hvm_msr {
    __u32 count;
    struct hvm_one_msr {
        __u32 index;
        __u32 _rsvd;
        __u64 val;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    } msr[];
#elif defined(__GNUC__)
    } msr[0];
#else
    } msr[1 /* variable size */];
#endif
};

#define CPU_MSR_CODE  20

/* 
 * Largest type-code in use
 */
#define HVM_SAVE_CODE_MAX 20

#endif /* __XEN_PUBLIC_HVM_SAVE_X86_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
