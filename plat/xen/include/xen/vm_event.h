/******************************************************************************
 * vm_event.h
 *
 * Memory event common structures.
 *
 * Copyright (c) 2009 by Citrix Systems, Inc. (Patrick Colp)
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

#ifndef _XEN_PUBLIC_VM_EVENT_H
#define _XEN_PUBLIC_VM_EVENT_H

#include "xen.h"

#define VM_EVENT_INTERFACE_VERSION 0x00000002

#if defined(__XEN__) || defined(__XEN_TOOLS__)

#include "io/ring.h"

/*
 * Memory event flags
 */

/*
 * VCPU_PAUSED in a request signals that the vCPU triggering the event has been
 *  paused
 * VCPU_PAUSED in a response signals to unpause the vCPU
 */
#define VM_EVENT_FLAG_VCPU_PAUSED        (1 << 0)
/* Flags to aid debugging vm_event */
#define VM_EVENT_FLAG_FOREIGN            (1 << 1)
/*
 * The following flags can be set in response to a mem_access event.
 *
 * Emulate the fault-causing instruction (if set in the event response flags).
 * This will allow the guest to continue execution without lifting the page
 * access restrictions.
 */
#define VM_EVENT_FLAG_EMULATE            (1 << 2)
/*
 * Same as VM_EVENT_FLAG_EMULATE, but with write operations or operations
 * potentially having side effects (like memory mapped or port I/O) disabled.
 */
#define VM_EVENT_FLAG_EMULATE_NOWRITE    (1 << 3)
/*
 * Toggle singlestepping on vm_event response.
 * Requires the vCPU to be paused already (synchronous events only).
 */
#define VM_EVENT_FLAG_TOGGLE_SINGLESTEP  (1 << 4)
/*
 * Data is being sent back to the hypervisor in the event response, to be
 * returned by the read function when emulating an instruction.
 * This flag is only useful when combined with VM_EVENT_FLAG_EMULATE
 * and takes precedence if combined with VM_EVENT_FLAG_EMULATE_NOWRITE
 * (i.e. if both VM_EVENT_FLAG_EMULATE_NOWRITE and
 * VM_EVENT_FLAG_SET_EMUL_READ_DATA are set, only the latter will be honored).
 */
#define VM_EVENT_FLAG_SET_EMUL_READ_DATA (1 << 5)
/*
 * Deny completion of the operation that triggered the event.
 * Currently only useful for MSR and control-register write events.
 * Requires the vCPU to be paused already (synchronous events only).
 */
#define VM_EVENT_FLAG_DENY               (1 << 6)
/*
 * This flag can be set in a request or a response
 *
 * On a request, indicates that the event occurred in the alternate p2m
 * specified by the altp2m_idx request field.
 *
 * On a response, indicates that the VCPU should resume in the alternate p2m
 * specified by the altp2m_idx response field if possible.
 */
#define VM_EVENT_FLAG_ALTERNATE_P2M      (1 << 7)
/*
 * Set the vCPU registers to the values in the  vm_event response.
 * At the moment x86-only, applies to EAX-EDX, ESP, EBP, ESI, EDI, R8-R15,
 * EFLAGS, and EIP.
 * Requires the vCPU to be paused already (synchronous events only).
 */
#define VM_EVENT_FLAG_SET_REGISTERS      (1 << 8)
/*
 * Instruction cache is being sent back to the hypervisor in the event response
 * to be used by the emulator. This flag is only useful when combined with
 * VM_EVENT_FLAG_EMULATE and does not take presedence if combined with
 * VM_EVENT_FLAG_EMULATE_NOWRITE or VM_EVENT_FLAG_SET_EMUL_READ_DATA, (i.e.
 * if any of those flags are set, only those will be honored).
 */
#define VM_EVENT_FLAG_SET_EMUL_INSN_DATA (1 << 9)
/*
 * Have a one-shot VM_EVENT_REASON_INTERRUPT event sent for the first
 * interrupt pending after resuming the VCPU.
 */
#define VM_EVENT_FLAG_GET_NEXT_INTERRUPT (1 << 10)

/*
 * Reasons for the vm event request
 */

/* Default case */
#define VM_EVENT_REASON_UNKNOWN                 0
/* Memory access violation */
#define VM_EVENT_REASON_MEM_ACCESS              1
/* Memory sharing event */
#define VM_EVENT_REASON_MEM_SHARING             2
/* Memory paging event */
#define VM_EVENT_REASON_MEM_PAGING              3
/* A control register was updated */
#define VM_EVENT_REASON_WRITE_CTRLREG           4
/* An MSR was updated. */
#define VM_EVENT_REASON_MOV_TO_MSR              5
/* Debug operation executed (e.g. int3) */
#define VM_EVENT_REASON_SOFTWARE_BREAKPOINT     6
/* Single-step (e.g. MTF) */
#define VM_EVENT_REASON_SINGLESTEP              7
/* An event has been requested via HVMOP_guest_request_vm_event. */
#define VM_EVENT_REASON_GUEST_REQUEST           8
/* A debug exception was caught */
#define VM_EVENT_REASON_DEBUG_EXCEPTION         9
/* CPUID executed */
#define VM_EVENT_REASON_CPUID                   10
/*
 * Privileged call executed (e.g. SMC).
 * Note: event may be generated even if SMC condition check fails on some CPUs.
 *       As this behavior is CPU-specific, users are advised to not rely on it.
 *       These kinds of events will be filtered out in future versions.
 */
#define VM_EVENT_REASON_PRIVILEGED_CALL         11
/* An interrupt has been delivered. */
#define VM_EVENT_REASON_INTERRUPT               12
/* A descriptor table register was accessed. */
#define VM_EVENT_REASON_DESCRIPTOR_ACCESS       13
/* Current instruction is not implemented by the emulator */
#define VM_EVENT_REASON_EMUL_UNIMPLEMENTED      14

/* Supported values for the vm_event_write_ctrlreg index. */
#define VM_EVENT_X86_CR0    0
#define VM_EVENT_X86_CR3    1
#define VM_EVENT_X86_CR4    2
#define VM_EVENT_X86_XCR0   3

/*
 * Using custom vCPU structs (i.e. not hvm_hw_cpu) for both x86 and ARM
 * so as to not fill the vm_event ring buffer too quickly.
 */
struct vm_event_regs_x86 {
    __u64 rax;
    __u64 rcx;
    __u64 rdx;
    __u64 rbx;
    __u64 rsp;
    __u64 rbp;
    __u64 rsi;
    __u64 rdi;
    __u64 r8;
    __u64 r9;
    __u64 r10;
    __u64 r11;
    __u64 r12;
    __u64 r13;
    __u64 r14;
    __u64 r15;
    __u64 rflags;
    __u64 dr7;
    __u64 rip;
    __u64 cr0;
    __u64 cr2;
    __u64 cr3;
    __u64 cr4;
    __u64 sysenter_cs;
    __u64 sysenter_esp;
    __u64 sysenter_eip;
    __u64 msr_efer;
    __u64 msr_star;
    __u64 msr_lstar;
    __u64 fs_base;
    __u64 gs_base;
    __u32 cs_arbytes;
    __u32 _pad;
};

/*
 * Only the register 'pc' can be set on a vm_event response using the
 * VM_EVENT_FLAG_SET_REGISTERS flag.
 */
struct vm_event_regs_arm {
    __u64 ttbr0;
    __u64 ttbr1;
    __u64 ttbcr;
    __u64 pc;
    __u32 cpsr;
    __u32 _pad;
};

/*
 * mem_access flag definitions
 *
 * These flags are set only as part of a mem_event request.
 *
 * R/W/X: Defines the type of violation that has triggered the event
 *        Multiple types can be set in a single violation!
 * GLA_VALID: If the gla field holds a guest VA associated with the event
 * FAULT_WITH_GLA: If the violation was triggered by accessing gla
 * FAULT_IN_GPT: If the violation was triggered during translating gla
 */
#define MEM_ACCESS_R                (1 << 0)
#define MEM_ACCESS_W                (1 << 1)
#define MEM_ACCESS_X                (1 << 2)
#define MEM_ACCESS_RWX              (MEM_ACCESS_R | MEM_ACCESS_W | MEM_ACCESS_X)
#define MEM_ACCESS_RW               (MEM_ACCESS_R | MEM_ACCESS_W)
#define MEM_ACCESS_RX               (MEM_ACCESS_R | MEM_ACCESS_X)
#define MEM_ACCESS_WX               (MEM_ACCESS_W | MEM_ACCESS_X)
#define MEM_ACCESS_GLA_VALID        (1 << 3)
#define MEM_ACCESS_FAULT_WITH_GLA   (1 << 4)
#define MEM_ACCESS_FAULT_IN_GPT     (1 << 5)

struct vm_event_mem_access {
    __u64 gfn;
    __u64 offset;
    __u64 gla;   /* if flags has MEM_ACCESS_GLA_VALID set */
    __u32 flags; /* MEM_ACCESS_* */
    __u32 _pad;
};

struct vm_event_write_ctrlreg {
    __u32 index;
    __u32 _pad;
    __u64 new_value;
    __u64 old_value;
};

struct vm_event_singlestep {
    __u64 gfn;
};

struct vm_event_debug {
    __u64 gfn;
    __u32 insn_length;
    __u8 type;        /* HVMOP_TRAP_* */
    __u8 _pad[3];
};

struct vm_event_mov_to_msr {
    __u64 msr;
    __u64 value;
};

#define VM_EVENT_DESC_IDTR           1
#define VM_EVENT_DESC_GDTR           2
#define VM_EVENT_DESC_LDTR           3
#define VM_EVENT_DESC_TR             4

struct vm_event_desc_access {
    union {
        struct {
            __u32 instr_info;         /* VMX: VMCS Instruction-Information */
            __u32 _pad1;
            __u64 exit_qualification; /* VMX: VMCS Exit Qualification */
        } vmx;
        struct {
            __u64 exitinfo;           /* SVM: VMCB EXITINFO */
            __u64 _pad2;
        } svm;
    } arch;
    __u8 descriptor;                  /* VM_EVENT_DESC_* */
    __u8 is_write;
    __u8 _pad[6];
};

struct vm_event_cpuid {
    __u32 insn_length;
    __u32 leaf;
    __u32 subleaf;
    __u32 _pad;
};

struct vm_event_interrupt_x86 {
    __u32 vector;
    __u32 type;
    __u32 error_code;
    __u32 _pad;
    __u64 cr2;
};

#define MEM_PAGING_DROP_PAGE       (1 << 0)
#define MEM_PAGING_EVICT_FAIL      (1 << 1)

struct vm_event_paging {
    __u64 gfn;
    __u32 p2mt;
    __u32 flags;
};

struct vm_event_sharing {
    __u64 gfn;
    __u32 p2mt;
    __u32 _pad;
};

struct vm_event_emul_read_data {
    __u32 size;
    /* The struct is used in a union with vm_event_regs_x86. */
    __u8  data[sizeof(struct vm_event_regs_x86) - sizeof(__u32)];
};

struct vm_event_emul_insn_data {
    __u8 data[16]; /* Has to be completely filled */
};

typedef struct vm_event_st {
    __u32 version;   /* VM_EVENT_INTERFACE_VERSION */
    __u32 flags;     /* VM_EVENT_FLAG_* */
    __u32 reason;    /* VM_EVENT_REASON_* */
    __u32 vcpu_id;
    __u16 altp2m_idx; /* may be used during request and response */
    __u16 _pad[3];

    union {
        struct vm_event_paging                mem_paging;
        struct vm_event_sharing               mem_sharing;
        struct vm_event_mem_access            mem_access;
        struct vm_event_write_ctrlreg         write_ctrlreg;
        struct vm_event_mov_to_msr            mov_to_msr;
        struct vm_event_desc_access           desc_access;
        struct vm_event_singlestep            singlestep;
        struct vm_event_debug                 software_breakpoint;
        struct vm_event_debug                 debug_exception;
        struct vm_event_cpuid                 cpuid;
        union {
            struct vm_event_interrupt_x86     x86;
        } interrupt;
    } u;

    union {
        union {
            struct vm_event_regs_x86 x86;
            struct vm_event_regs_arm arm;
        } regs;

        union {
            struct vm_event_emul_read_data read;
            struct vm_event_emul_insn_data insn;
        } emul;
    } data;
} vm_event_request_t, vm_event_response_t;

DEFINE_RING_TYPES(vm_event, vm_event_request_t, vm_event_response_t);

#endif /* defined(__XEN__) || defined(__XEN_TOOLS__) */
#endif /* _XEN_PUBLIC_VM_EVENT_H */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
