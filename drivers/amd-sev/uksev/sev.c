/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#define UK_DEBUG

#include "uk/asm/sev.h"
#include "kvm-x86/serial_console.h"
#include "uk/arch/lcpu.h"
#include "uk/arch/paging.h"
#include "uk/asm/sev-ghcb.h"
#include "uk/asm/svm.h"
#include "uk/bitops.h"
#include "uk/isr/string.h"
#include "uk/plat/bootstrap.h"
#include "uk/plat/io.h"
#include "uk/plat/paging.h"
#include <x86/cpu.h>
#include "x86/desc.h"
#include "x86/traps.h"
#include <stdio.h>
#include <uk/sev.h>
#include <errno.h>
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/page.h>

#include <kvm-x86/traps.h>

#include <uk/event.h>

#include "decoder.h"

/* Boot GDT and IDT to setup the early VC handler */
static __align(8) struct seg_desc32 boot_cpu_gdt64[GDT_NUM_ENTRIES];
static struct seg_gate_desc64 sev_boot_idt[IDT_NUM_ENTRIES] __align(8);
static struct desc_table_ptr64 boot_idtptr;
static struct ghcb ghcb_page __align(__PAGE_SIZE);


/* Debug printing is only available after GHCB is initialized. */
int ghcb_initialized = 1;

static inline void uk_sev_ghcb_wrmsrl(__u64 value)
{
	wrmsrl(SEV_ES_MSR_GHCB, value);
}
static inline __u64 uk_sev_ghcb_rdmsrl()
{
	return rdmsrl(SEV_ES_MSR_GHCB);
}

static inline __u64 uk_sev_ghcb_msr_invoke(__u64 value)
{
	uk_sev_ghcb_wrmsrl(value);
	vmgexit();
	return uk_sev_ghcb_rdmsrl();
}

int uk_sev_ghcb_initialized(void) {
#ifdef CONFIG_X86_AMD64_FEAT_SEV_ES
	return ghcb_initialized;
#else
	return 1;
#endif
}
void uk_sev_terminate(int set, int reason)
{
	uk_sev_ghcb_wrmsrl(SEV_GHCB_MSR_TERM_REQ_VAL(set, reason));
	vmgexit();
}

int uk_sev_ghcb_vmm_call(struct ghcb *ghcb, __u64 exitcode, __u64 exitinfo1,
			 __u64 exitinfo2)
{
	GHCB_SAVE_AREA_SET_FIELD(ghcb, sw_exitcode, exitcode);
	GHCB_SAVE_AREA_SET_FIELD(ghcb, sw_exitinfo1, exitinfo1);
	GHCB_SAVE_AREA_SET_FIELD(ghcb, sw_exitinfo2, exitinfo2);
	ghcb->ghcb_usage = SEV_GHCB_USAGE_DEFAULT;

	/* TODO: Negotiate ghcb protocol version */
	/* ghcb->protocol_version = 1; */
	uk_sev_ghcb_wrmsrl(ukplat_virt_to_phys(ghcb));
	vmgexit();

	/* TODO: Verify VMM return */
	return 0;
};

static inline int _uk_sev_ghcb_cpuid_reg(int reg_idx, int fn, __u32 *reg)
{
	__u64 val, code;

	val = uk_sev_ghcb_msr_invoke(SEV_GHCB_MSR_CPUID_REQ_VAL(reg_idx, fn));
	code = SEV_GHCB_MSR_RESP_CODE(val);
	if (code != SEV_GHCB_MSR_CPUID_RESP) {
		return -1;
	}

	*reg = (val >> 32);
	return 0;
}

static inline int uk_sev_ghcb_cpuid(__u32 fn, __unused __u32 sub_fn, __u32 *eax,
				    __u32 *ebx, __u32 *ecx, __u32 *edx)
{
	int rc;

	rc = _uk_sev_ghcb_cpuid_reg(SEV_GHCB_MSR_CPUID_REQ_RAX, fn, eax);
	rc = rc ? rc
		: _uk_sev_ghcb_cpuid_reg(SEV_GHCB_MSR_CPUID_REQ_RBX, fn, ebx);
	rc = rc ? rc
		: _uk_sev_ghcb_cpuid_reg(SEV_GHCB_MSR_CPUID_REQ_RCX, fn, ecx);
	rc = rc ? rc
		: _uk_sev_ghcb_cpuid_reg(SEV_GHCB_MSR_CPUID_REQ_RDX, fn, edx);
	return rc;
}

#define COM1 0x3f8
#define COM1_DATA (COM1 + 0)
#define COM1_STATUS (COM1 + 5)
#define MAX_SEV_PRINT_LEN 512

/*
 * Emulation of serial print, so that #VC is not triggered.
 */
// #define SERIAL_PRINTF 1
static void uk_sev_serial_printf(struct ghcb *ghcb, const char* fmt, ...){
#if SERIAL_PRINTF
	if (!ghcb_initialized)
		return;

	char buf[MAX_SEV_PRINT_LEN];
	unsigned long orig_rax, orig_rdx, orig_rax_valid, orig_rdx_valid;

	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	__u64 exitinfo1 = 0;
	exitinfo1 |= UK_SEV_IOIO_TYPE_OUT;
	exitinfo1 |= UK_SEV_IOIO_TYPE_OUT;
	exitinfo1 |= UK_SEV_IOIO_SEG(UK_SEV_IOIO_SEG_DS);
	exitinfo1 |= UK_SEV_IOIO_PORT(COM1_DATA);
	exitinfo1 |= UK_SEV_IOIO_SZ8;
	exitinfo1 |= UK_SEV_IOIO_A16;

	/* Backup rax and rdx if they are set previously */
	orig_rax_valid = GHCB_SAVE_AREA_GET_VALID(ghcb, rax);
	orig_rdx_valid = GHCB_SAVE_AREA_GET_VALID(ghcb, rdx);

	if (orig_rax_valid)
		orig_rax = ghcb->save_area.rax;
	if (orig_rdx_valid)
		orig_rdx = ghcb->save_area.rdx;

	for (int i = 0; i < MAX_SEV_PRINT_LEN; i++) {
		if (buf[i] == '\0')
			break;
		if (buf[i] == '\n') {
			GHCB_SAVE_AREA_SET_FIELD(ghcb, rax,
						 '\r' & (UK_BIT(8) - 1));
			uk_sev_ghcb_vmm_call(ghcb, SVM_VMEXIT_IOIO, exitinfo1,
					     0);
		}

		GHCB_SAVE_AREA_SET_FIELD(ghcb, rax, buf[i] & (UK_BIT(8) - 1));
		uk_sev_ghcb_vmm_call(ghcb, SVM_VMEXIT_IOIO, exitinfo1, 0);
	}

	/* Restore rax and rdx */
	if (orig_rax_valid)
		GHCB_SAVE_AREA_SET_FIELD(ghcb, rax, orig_rax);
	if (orig_rdx_valid)
		GHCB_SAVE_AREA_SET_FIELD(ghcb, rdx, orig_rdx);
#endif

}

int do_vmm_comm_exception_no_ghcb(struct __regs *regs,
				   unsigned long error_code)
{
	__u32 fn = regs->rax;
	if (error_code == SVM_VMEXIT_CPUID) {
		__u32 eax, ebx = 0, ecx, edx;
		int rc;
		rc = uk_sev_ghcb_cpuid(fn, 0, &eax, &ebx, &ecx, &edx);

		if (unlikely(rc)) {
			return rc;
		}

		regs->rax = eax;
		regs->rbx = ebx;
		regs->rcx = ecx;
		regs->rdx = edx;

		/* Advance RIP by 2 bytes (CPUID opcode) */
		regs->rip += 2;
		return 0;
	}

	uk_sev_terminate(1, error_code);
	/* Other error code are not supported */
	return -ENOTSUP;
}

int uk_sev_set_pages_state(__vaddr_t vstart, __paddr_t pstart, unsigned long num_pages,
			   int page_state)
{

	vstart = PAGE_ALIGN_DOWN(vstart);
	pstart = PAGE_ALIGN_DOWN(pstart);

	for (unsigned long i = 0; i < num_pages; i++){
		int rc;
		__u64 val;
		__paddr_t paddr = pstart + i * PAGE_SIZE;
		__paddr_t vaddr = vstart + i * PAGE_SIZE;

		if (page_state == SEV_GHCB_MSR_SNP_PSC_PG_SHARED) {
			uk_pr_info("Pvalidating vaddr 0x%lx paddr 0x%lx shared\n", vaddr,
				   paddr);

			/* Un-validate page to make it shared */
			rc = pvalidate(vaddr, PVALIDATE_PAGE_SIZE_4K, 0);
			if (unlikely(rc))
			{
				uk_pr_warn("pvalidate failed, error code: %d.\n", rc);
				return rc;
			}
		}

		uk_pr_info("Requesting PSC\n");
		val = uk_sev_ghcb_msr_invoke(SEV_GHCB_MSR_SNP_PSC_REQ_VAL(
		    page_state, paddr >> PAGE_SHIFT));

		if (SEV_GHCB_MSR_RESP_CODE(val) != SEV_GHCB_MSR_SNP_PSC_RESP)
			return -1;

		if (SEV_GHCB_MSR_SNP_PSC_RESP_VAL(val)) {
			uk_pr_warn("PSC request failed, error code: 0x%lx.\n",
				   val);
			return -1;
		}

		if (page_state == SEV_GHCB_MSR_SNP_PSC_PG_PRIVATE) {
			uk_pr_info("Pvalidating vaddr 0x%lx paddr 0x%lx private\n", vaddr, paddr);

			rc = pvalidate(vaddr, PVALIDATE_PAGE_SIZE_4K, 1);
			if (unlikely(rc))
			{
				uk_pr_warn("pvalidate failed, error code: %d.\n", rc);
				return rc;
			}
		}
	}

	return 0;
}


int uk_sev_set_memory_private(__vaddr_t addr, unsigned long num_pages)
{
	UK_ASSERT(PAGE_ALIGNED(addr));
	unsigned long prot;
	int rc;

	uk_pr_info("Setting 0x%lx, %lu pages to private\n", addr, num_pages);
	prot = PAGE_ATTR_PROT_RW | PAGE_ATTR_ENCRYPT;
	rc = ukplat_page_set_attr(ukplat_pt_get_active(), addr, num_pages, prot, 0);

	if (unlikely(rc))
		return rc;

#ifdef CONFIG_X86_AMD64_FEAT_SEV_SNP
	rc = uk_sev_set_pages_state(addr, ukplat_virt_to_phys((void*)addr), num_pages,
	 			    SEV_GHCB_MSR_SNP_PSC_PG_PRIVATE);
	if (unlikely(rc))
		return rc;
#endif

	memset((void *)addr, 0, num_pages * PAGE_SIZE);
	return 0;
}

int uk_sev_set_memory_shared(__vaddr_t addr, unsigned long num_pages)
{
	UK_ASSERT(PAGE_ALIGNED(addr));

	unsigned long prot;
	int rc;

	uk_pr_info("Setting 0x%lx, %lu pages to shared\n", addr, num_pages);

	/* Clearing memory before sharing the page. This also makes sure the PTE
	 * is present */
	memset((void *)addr, 0, num_pages * PAGE_SIZE);

#ifdef CONFIG_X86_AMD64_FEAT_SEV_SNP
	rc = uk_sev_set_pages_state(addr, ukplat_virt_to_phys((void *)addr),
				    num_pages, SEV_GHCB_MSR_SNP_PSC_PG_SHARED);
	if (unlikely(rc))
		return rc;
#endif

	prot = PAGE_ATTR_PROT_RW;
	rc = ukplat_page_set_attr(ukplat_pt_get_active(), addr, num_pages, prot,
				  0);

	if (unlikely(rc))
		return rc;

	return 0;
}

static inline int _uk_sev_ghcb_gpa_register(__paddr_t paddr)
{
	__u64 pfn, val;

	pfn = paddr >> __PAGE_SHIFT;
	val = uk_sev_ghcb_msr_invoke(SEV_GHCB_MSR_REG_GPA_REQ_VAL(pfn));

	if (SEV_GHCB_MSR_RESP_CODE(val) != SEV_GHCB_MSR_REG_GPA_RESP
	    && SEV_GHCB_MSR_REG_GPA_RESP_PFN(val) != pfn))
		return -1;

	return 0;
}

/* TODO: GHCB should be setup per-cpu */
int uk_sev_setup_ghcb(void)
{
#ifdef CONFIG_X86_AMD64_FEAT_SEV_ES
	int rc;
	__paddr_t ghcb_paddr;

	rc = uk_sev_set_memory_shared((__vaddr_t)&ghcb_page,
				      sizeof(struct ghcb) / __PAGE_SIZE);

	if (unlikely(rc))
		return rc;

	memset(&ghcb_page, 0, sizeof(struct ghcb));

	ghcb_paddr = ukplat_virt_to_phys(&ghcb_page);

	rc = _uk_sev_ghcb_gpa_register(ghcb_paddr);
	if (unlikely(rc))
		return rc;

	/* Fixup the idt with the VC handler that uses the GHCB  */
	traps_table_ghcb_vc_handler_init();

	ghcb_initialized = 1;

	/* An instruction decoder is needed to handle I/O-related #VC */
	rc = uk_sev_decoder_init();

	if (unlikely(rc)) {
		UK_CRASH("Fail initializing instruction decoder\n");
		return -1;
	}
#endif /* CONFIG_X86_AMD64_FEAT_SEV_ES */

	return 0;
}

static void uk_sev_boot_gdt_init(void)
{
	volatile struct desc_table_ptr64 gdtptr; /* needs to be volatile so
						  * setting its values is not
						  * optimized out
						  */

	boot_cpu_gdt64[GDT_DESC_CODE].raw = GDT_DESC_CODE64_VAL;
	boot_cpu_gdt64[GDT_DESC_DATA].raw = GDT_DESC_DATA64_VAL;

	gdtptr.limit = sizeof(boot_cpu_gdt64) - 1;
	gdtptr.base = (__u64)&boot_cpu_gdt64;
	__asm__ goto(
	    /* Load the global descriptor table */
	    "lgdt	%0\n"

	    /* Perform a far return to enable the new CS */
	    "leaq	%l[jump_to_new_cs](%%rip), %%rax\n"

	    "pushq	%1\n"
	    "pushq	%%rax\n"
	    "lretq\n"
	    :
	    : "m"(gdtptr), "i"(GDT_DESC_OFFSET(GDT_DESC_CODE))
	    : "rax", "memory"
	    : jump_to_new_cs);
jump_to_new_cs:
	__asm__ __volatile__(
	    /* Update remaining segment registers */
	    "movl	%0, %%es\n"
	    "movl	%0, %%ss\n"
	    "movl	%0, %%ds\n"

	    /* Initialize fs and gs to 0 */
	    "movl	%1, %%fs\n"
	    "movl	%1, %%gs\n"
	    :
	    : "r"(GDT_DESC_OFFSET(GDT_DESC_DATA)), "r"(0));
	return;
}

static void uk_sev_boot_idt_fillgate(unsigned int num, void *fun,
				     unsigned int ist)
{
	struct seg_gate_desc64 *desc = &sev_boot_idt[num];

	/*
	 * All gates are interrupt gates, all handlers run with interrupts off.
	 */
	desc->offset_hi = (__u64)fun >> 16;
	desc->offset_lo = (__u64)fun & 0xffff;
	desc->selector = IDT_DESC_OFFSET(IDT_DESC_CODE);
	desc->ist = ist;
	desc->type = IDT_DESC_TYPE_INTR;
	desc->dpl = IDT_DESC_DPL_KERNEL;
	desc->p = 1;
}

static void uk_sev_boot_idt_init(void)
{
	__asm__ __volatile__("lidt %0" ::"m"(boot_idtptr));
}

static void uk_sev_boot_traps_table_init(void)
{
	uk_sev_boot_idt_fillgate(TRAP_vmm_comm_exception,
				 &ASM_TRAP_SYM(vmm_comm_exception_no_ghcb), 0);
	boot_idtptr.limit = sizeof(sev_boot_idt) - 1;
	boot_idtptr.base = (__u64)&sev_boot_idt;
}

static inline int
uk_sev_ioio_exitinfo1_init(struct uk_sev_decoded_inst *instruction,
			   struct __regs *regs, __u64 *exitinfo1)
{
	/* Encode the instruction type */
	switch (instruction->opcode) {
	/* INS */
	case 0x6c:
	case 0x6d:
		*exitinfo1 |= UK_SEV_IOIO_TYPE_IN;
		*exitinfo1 |= UK_SEV_IOIO_STR;
		*exitinfo1 |= UK_SEV_IOIO_SEG(UK_SEV_IOIO_SEG_ES);
		*exitinfo1 |= UK_SEV_IOIO_PORT(regs->rdx);
		break;

	/* OUTS */
	case 0x6e:
	case 0x6f:
		*exitinfo1 |= UK_SEV_IOIO_TYPE_OUT;
		*exitinfo1 |= UK_SEV_IOIO_STR;
		*exitinfo1 |= UK_SEV_IOIO_SEG(UK_SEV_IOIO_SEG_DS);
		*exitinfo1 |= UK_SEV_IOIO_PORT(regs->rdx);
		break;
	/* IN imm */
	case 0xe4:
	case 0xe5:
		*exitinfo1 |= UK_SEV_IOIO_TYPE_IN;
		*exitinfo1 |= UK_SEV_IOIO_PORT((__u8)instruction->immediate);
		break;
	/* OUT imm */
	case 0xe6:
	case 0xe7:
		*exitinfo1 |= UK_SEV_IOIO_TYPE_OUT;
		*exitinfo1 |= UK_SEV_IOIO_PORT((__u8)instruction->immediate);
		break;
	/* IN reg */
	case 0xec:
	case 0xed:
		*exitinfo1 |= UK_SEV_IOIO_TYPE_IN;
		*exitinfo1 |= UK_SEV_IOIO_SEG(UK_SEV_IOIO_SEG_ES);
		*exitinfo1 |= UK_SEV_IOIO_PORT(regs->rdx);
		break;
	/* OUT reg */
	case 0xee:
	case 0xef:
		*exitinfo1 |= UK_SEV_IOIO_TYPE_OUT;
		*exitinfo1 |= UK_SEV_IOIO_SEG(UK_SEV_IOIO_SEG_DS);
		*exitinfo1 |= UK_SEV_IOIO_PORT(regs->rdx);
		break;
	default:
		return -1;
	}

	/* Encode the operand size */
	switch (instruction->operand_width) {
	case 8:
		*exitinfo1 |= UK_SEV_IOIO_SZ8;
		break;
	case 16:
		*exitinfo1 |= UK_SEV_IOIO_SZ16;
		break;
	case 32:
		*exitinfo1 |= UK_SEV_IOIO_SZ32;
		break;
	default:
		return -1;
	}

	/* Encode the address size */
	switch (instruction->address_width) {
	case 16:
		*exitinfo1 |= UK_SEV_IOIO_A16;
		break;
	case 32:
		*exitinfo1 |= UK_SEV_IOIO_A32;
		break;
	case 64:
		*exitinfo1 |= UK_SEV_IOIO_A64;
		break;
	default:
		return -1;
	}

	/* Encode rep prefix */
	if (instruction->has_rep) {
		*exitinfo1 |= UK_SEV_IOIO_REP;
	}
	return 0;
}

static int uk_sev_handle_ioio_exitinfo1(struct uk_sev_decoded_inst *instruction,
					struct __regs *regs, struct ghcb *ghcb,
					__u64 exitinfo1)
{
	__u64 exitinfo2 = 0;
	if (exitinfo1 & UK_SEV_IOIO_STR) {

		/* We only handle reg and imm-based IO for now */
		return -ENOTSUP;
	} else { /* Non-string IN/OUT */
		__u8 addr_bits;
		if (exitinfo1 & UK_SEV_IOIO_A16)
			addr_bits = 16;
		else if (exitinfo1 & UK_SEV_IOIO_A32)
			addr_bits = 32;
		else
			addr_bits = 64;

		if (!(exitinfo1 & UK_SEV_IOIO_TYPE_IN)) {
			GHCB_SAVE_AREA_SET_FIELD(
			    ghcb, rax, regs->rax & (UK_BIT(addr_bits) - 1));
		}
		uk_sev_ghcb_vmm_call(ghcb, SVM_VMEXIT_IOIO, exitinfo1,
				     exitinfo2);

		if (exitinfo1 & UK_SEV_IOIO_TYPE_IN) {
			regs->rax =
			    (ghcb->save_area.rax & (UK_BIT(addr_bits) - 1));
		}
	}
	return 0;
}

static int uk_sev_handle_ioio(struct __regs *regs, struct ghcb *ghcb)
{
	struct uk_sev_decoded_inst instruction;
	__u64 exitinfo1 = 0;
	int rc;
	rc = uk_sev_decode_inst(regs->rip, &instruction);
	if (unlikely(rc)) {
		UK_CRASH("Failed decoding instruction\n");
	}

	rc = uk_sev_ioio_exitinfo1_init(&instruction, regs, &exitinfo1);
	if (unlikely(rc)) {
		UK_CRASH("Failed building exitinfo1\n");
	}

	rc = uk_sev_handle_ioio_exitinfo1(&instruction, regs, ghcb, exitinfo1);
	if (unlikely(rc)) {
		UK_CRASH("Failed handling exitinfo1\n");
	}

	/* Now skip over the emulated instruction */
	regs->rip += instruction.length;
	return 0;
}

static int uk_sev_do_mmio(struct ghcb *ghcb,int is_read){

}
static int uk_sev_handle_mmio_inst(struct __regs *regs, struct ghcb *ghcb,
				   struct uk_sev_decoded_inst *instruction)
{
	unsigned long *reg_ref, *mem_ref;
	__u64 disp = 0;
	int rc, sz;
	int is_read = 0;

	char buffer[512];
	memset_isr(buffer, 0, 512);
	uk_sev_pr_instruction(instruction, buffer, 512);
	uk_sev_serial_printf(ghcb, "Handling MMIO %s\n", buffer);
	uk_sev_serial_printf(ghcb, "opcode: 0x%x\n", instruction->opcode);


	/* uk_sev_pr_instruction(instruction); */
	/* uk_pr_info("Opcode: 0x%" __PRIx64 "\n", instruction->opcode); */

	__u64 exitcode, exitinfo1, exitinfo2;
	__paddr_t ghcb_phys, shared_buffer;
	switch (instruction->opcode) {
	/* MOV r, r/m: MMIO write reg to memory */
	case 0x88:
	case 0x89:
		rc = uk_sev_inst_get_reg_operand(instruction, regs, &reg_ref);
		if (rc)
			UK_CRASH("Failed getting reg operand\n");

		rc = uk_sev_inst_get_mem_reg_operand(instruction, regs,
						     &mem_ref);

		rc = uk_sev_inst_get_displacement(instruction, &disp);
		sz = instruction->operand_width / 8;


		exitcode = SVM_VMGEXIT_MMIO_WRITE;
		exitinfo1 = (__u64)ukplat_virt_to_phys((void *)*mem_ref) + disp;
		exitinfo2 = sz;

		/* shared_buffer = ukplat_virt_to_phys(ghcb->shared_buffer); */
		ghcb_phys = ukplat_virt_to_phys(ghcb);
		shared_buffer =
		    ghcb_phys + __offsetof(struct ghcb, shared_buffer);
		uk_sev_serial_printf(ghcb, "shared buffer: 0x%lx\n", shared_buffer);
		uk_sev_serial_printf(ghcb, "exit_reason: 0x%lx\n", exitcode);
		uk_sev_serial_printf(ghcb, "ghcb: 0x%lx\n", ghcb_phys);


		/* shared_buffer = */
		/*     ghcb_phys + __offsetof(struct ghcb, shared_buffer); */

		memcpy_isr(ghcb->shared_buffer, reg_ref, sz);
		uk_sev_serial_printf(
		    ghcb, "Requesting write: %d size, value %x to 0x%lx\n", exitinfo2,
		    *reg_ref, exitinfo1);

		GHCB_SAVE_AREA_SET_FIELD(ghcb, sw_scratch, shared_buffer);

		uk_sev_ghcb_vmm_call(ghcb, exitcode, exitinfo1, exitinfo2);
		break;

	/* MOV r/m, r: MMIO read memory to reg */
	case 0x8a:
	case 0x8b:
		is_read = 1;
		rc = uk_sev_inst_get_reg_operand(instruction, regs, &reg_ref);
		if (rc)
			UK_CRASH("Failed getting reg operand\n");

		rc = uk_sev_inst_get_mem_reg_operand(instruction, regs,
						     &mem_ref);

		rc = uk_sev_inst_get_displacement(instruction, &disp);
		sz = instruction->operand_width / 8;

		exitcode = SVM_VMGEXIT_MMIO_READ;
		exitinfo1 = (__u64)ukplat_virt_to_phys((void *)*mem_ref) + disp;
		exitinfo2 = sz;
		ghcb_phys = ukplat_virt_to_phys(ghcb);
		shared_buffer =
		    ghcb_phys + __offsetof(struct ghcb, shared_buffer);
		uk_sev_serial_printf(ghcb, "shared buffer: 0x%lx\n", shared_buffer);

		/* shared_buffer = ukplat_virt_to_phys(ghcb->shared_buffer); */
		GHCB_SAVE_AREA_SET_FIELD(ghcb, sw_scratch, shared_buffer);

		uk_sev_ghcb_vmm_call(ghcb, exitcode, exitinfo1, exitinfo2);

		uk_sev_serial_printf(ghcb,
				     "Requesting read: %d size, to 0x%lx\n",
				     exitinfo2, exitinfo1);

		uk_sev_serial_printf(ghcb, "Read result: 0x%lx\n",
				     *(unsigned long *)ghcb->shared_buffer);

		memcpy_isr(reg_ref, ghcb->shared_buffer, sz);
		break;

	default:
		uk_sev_terminate(9,9);
		return -ENOTSUP;
	}

	return 0;
}

static int uk_sev_handle_mmio(struct __regs *regs, struct ghcb *ghcb)
{
	struct uk_sev_decoded_inst instruction;
	int rc;

	rc = uk_sev_decode_inst(regs->rip, &instruction);
	if (unlikely(rc))
		return rc;

	rc = uk_sev_handle_mmio_inst(regs, ghcb, &instruction);
	if (unlikely(rc))
		return rc;

	regs->rip += instruction.length;
	return 0;
}

static int uk_sev_handle_msr(struct __regs *regs, struct ghcb *ghcb)
{
	__u64 exitcode, exitinfo1;
	int rc;
	struct uk_sev_decoded_inst instruction;

	rc = uk_sev_decode_inst(regs->rip, &instruction);
	if (unlikely(rc)) {
		return rc;
	}

	switch (instruction.opcode) {
	case 0x30: /* WRMSR opcode */
		exitinfo1 = 1;
		break;
	case 0x32: /* RDMSR opcode */
		exitinfo1 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	exitcode = SVM_VMEXIT_MSR;
	GHCB_SAVE_AREA_SET_FIELD(ghcb, rcx, regs->rcx);

	if (exitinfo1) {
		GHCB_SAVE_AREA_SET_FIELD(ghcb, rax, regs->rax);
		GHCB_SAVE_AREA_SET_FIELD(ghcb, rdx, regs->rdx);
	}

	rc = uk_sev_ghcb_vmm_call(ghcb, exitcode, exitinfo1, 0);
	if (unlikely(rc))
		return rc;

	if (!exitinfo1) {
		regs->rax = ghcb->save_area.rax;
		regs->rdx = ghcb->save_area.rdx;
	}
	regs->rip += instruction.length;

	return 0;
}

static int uk_sev_handle_vc(void *data)
{
	struct ukarch_trap_ctx *ctx = (struct ukarch_trap_ctx *)data;

	int exit_code = ctx->error_code;
	struct ghcb *ghcb = &ghcb_page;

	int rc = 0;
	switch (exit_code) {
	case SVM_VMEXIT_CPUID:
		/* Reuse the no GHCB handler for now */
		do_vmm_comm_exception_no_ghcb(ctx->regs, exit_code);
		break;
	case SVM_VMEXIT_IOIO:
		rc = uk_sev_handle_ioio(ctx->regs, ghcb);
		break;
	case SVM_VMEXIT_NPF:
		rc = uk_sev_handle_mmio(ctx->regs, ghcb);
		break;
	case SVM_VMEXIT_MSR:
		rc = uk_sev_handle_msr(ctx->regs, ghcb);
		break;
	default:
		uk_sev_terminate(2, exit_code);
		return UK_EVENT_NOT_HANDLED;
	}

	if (unlikely(rc)) {
		uk_sev_terminate(3, rc);
		UK_CRASH("Failed handling #VC , ec = %d\n", exit_code);
		return UK_EVENT_NOT_HANDLED;
	}

	return UK_EVENT_HANDLED;
}

int uk_sev_mem_encrypt_init(void)
{
	__u32 eax, ebx, ecx, edx;
	__u32 encryption_bit;

	ukarch_x86_cpuid(0x8000001f, 0, &eax, &ebx, &ecx, &edx);
	encryption_bit = ebx & X86_AMD64_CPUID_EBX_MEM_ENCRYPTION_MASK;

	if (unlikely(encryption_bit != CONFIG_LIBUKSEV_PTE_MEM_ENCRYPT_BIT)) {
		UK_CRASH("Invalid encryption bit configuration, please set "
			 "it to %d in the config.\n",
			 encryption_bit);
	}

	return 0;
};

int uk_sev_cpu_features_check(void){
	__u32 eax, ebx, ecx, edx;

	ukarch_x86_cpuid(0x8000001f, 0, &eax, &ebx, &ecx, &edx);
	if (unlikely(!(eax & X86_AMD64_CPUID_EAX_SEV_ENABLED))) {
		uk_pr_crit("%s not supported.\n", "AMD SEV");
		return -ENOTSUP;
	}

#ifdef CONFIG_X86_AMD64_FEAT_SEV_ES
	if (unlikely(!(eax & X86_AMD64_CPUID_EAX_SEV_ES_ENABLED))) {
		uk_pr_crit("%s not supported.\n", "AMD SEV-ES");
		return -ENOTSUP;
	}
#endif /* CONFIG_X86_AMD64_FEAT_SEV_ES */

#ifdef CONFIG_X86_AMD64_FEAT_SEV_SNP
	if (unlikely(!(eax & X86_AMD64_CPUID_EAX_SEV_SNP_ENABLED))) {
		uk_pr_crit("%s not supported.\n", "AMD SEV-SNP");
		return -ENOTSUP;
	}
#endif /* CONFIG_X86_AMD64_FEAT_SEV_SNP */
	return 0;
}



int uk_sev_early_vc_handler_init(void)
{
	uk_sev_boot_traps_table_init();
	uk_sev_boot_gdt_init();
	uk_sev_boot_idt_init();
	ghcb_initialized = 0;
	return 0;
}



UK_EVENT_HANDLER(UKARCH_TRAP_VC, uk_sev_handle_vc);
