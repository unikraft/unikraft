/* SPDX-License-Identifier: MIT */
/*
 * arch-x86/cpufeatureset.h
 *
 * CPU featureset definitions
 *
 * Copyright (c) 2015, 2016 Citrix Systems, Inc.
 */

/*
 * There are two expected ways of including this header.
 *
 * 1) The "default" case (expected from tools etc).
 *
 * Simply #include <public/arch-x86/cpufeatureset.h>
 *
 * In this circumstance, normal header guards apply and the includer shall get
 * an enumeration in the XEN_X86_FEATURE_xxx namespace.
 *
 * 2) The special case where the includer provides XEN_CPUFEATURE() in scope.
 *
 * In this case, no inclusion guards apply and the caller is responsible for
 * their XEN_CPUFEATURE() being appropriate in the included context.
 */

/* SAF-8-safe inclusion procedure left to caller */
#ifndef XEN_CPUFEATURE

/*
 * Includer has not provided a custom XEN_CPUFEATURE().  Arrange for normal
 * header guards, an enum and constants in the XEN_X86_FEATURE_xxx namespace.
 */
#ifndef __XEN_PUBLIC_ARCH_X86_CPUFEATURESET_H__
#define __XEN_PUBLIC_ARCH_X86_CPUFEATURESET_H__

#define XEN_CPUFEATURESET_DEFAULT_INCLUDE

#define XEN_CPUFEATURE(name, value) XEN_X86_FEATURE_##name = value,
enum {

#endif /* __XEN_PUBLIC_ARCH_X86_CPUFEATURESET_H__ */
#endif /* !XEN_CPUFEATURE */


#ifdef XEN_CPUFEATURE
/*
 * A featureset is a bitmap of x86 features, represented as a collection of
 * 32bit words.
 *
 * Words are as specified in vendors programming manuals, and shall not
 * contain any synthesied values.  New words may be added to the end of
 * featureset.
 *
 * All featureset words currently originate from leaves specified for the
 * CPUID instruction, but this is not preclude other sources of information.
 */

/*
 * Attribute syntax:
 *
 * Attributes for a particular feature are provided as characters before the
 * first space in the comment immediately following the feature value.  Note -
 * none of these attributes form part of the Xen public ABI.
 *
 * Special: '!'
 *   This bit has special properties and is not a straight indication of a
 *   piece of new functionality.  Xen will handle these differently,
 *   and may override toolstack settings completely.
 *
 * Applicability to guests: 'A', 'S' or 'H'
 *   'A' = All guests.
 *   'S' = All HVM guests (not PV guests).
 *   'H' = HVM HAP guests (not PV or HVM Shadow guests).
 *   Upper case => Available by default
 *   Lower case => Can be opted-in to, but not available by default.
 *
 * Migration: '|'
 *   This bit should be visible to a guest if any anywhere it might run has
 *   the bit set.  i.e. it needs accumulating across the migration pool,
 *   rather than intersecting.
 */

/* Intel-defined CPU features, CPUID level 0x00000001.edx, word 0 */
XEN_CPUFEATURE(FPU,           0*32+ 0) /*A  Onboard FPU */
XEN_CPUFEATURE(VME,           0*32+ 1) /*S  Virtual Mode Extensions */
XEN_CPUFEATURE(DE,            0*32+ 2) /*A  Debugging Extensions */
XEN_CPUFEATURE(PSE,           0*32+ 3) /*S  Page Size Extensions */
XEN_CPUFEATURE(TSC,           0*32+ 4) /*A  Time Stamp Counter */
XEN_CPUFEATURE(MSR,           0*32+ 5) /*A  Model-Specific Registers, RDMSR, WRMSR */
XEN_CPUFEATURE(PAE,           0*32+ 6) /*A  Physical Address Extensions */
XEN_CPUFEATURE(MCE,           0*32+ 7) /*A  Machine Check Architecture */
XEN_CPUFEATURE(CX8,           0*32+ 8) /*A  CMPXCHG8 instruction */
XEN_CPUFEATURE(APIC,          0*32+ 9) /*!A Onboard APIC */
XEN_CPUFEATURE(SEP,           0*32+11) /*A  SYSENTER/SYSEXIT */
XEN_CPUFEATURE(MTRR,          0*32+12) /*S  Memory Type Range Registers */
XEN_CPUFEATURE(PGE,           0*32+13) /*S  Page Global Enable */
XEN_CPUFEATURE(MCA,           0*32+14) /*A  Machine Check Architecture */
XEN_CPUFEATURE(CMOV,          0*32+15) /*A  CMOV instruction (FCMOVCC and FCOMI too if FPU present) */
XEN_CPUFEATURE(PAT,           0*32+16) /*A  Page Attribute Table */
XEN_CPUFEATURE(PSE36,         0*32+17) /*S  36-bit PSEs */
XEN_CPUFEATURE(CLFLUSH,       0*32+19) /*A  CLFLUSH instruction */
XEN_CPUFEATURE(DS,            0*32+21) /*   Debug Store */
XEN_CPUFEATURE(ACPI,          0*32+22) /*A  ACPI via MSR */
XEN_CPUFEATURE(MMX,           0*32+23) /*A  Multimedia Extensions */
XEN_CPUFEATURE(FXSR,          0*32+24) /*A  FXSAVE and FXRSTOR instructions */
XEN_CPUFEATURE(SSE,           0*32+25) /*A  Streaming SIMD Extensions */
XEN_CPUFEATURE(SSE2,          0*32+26) /*A  Streaming SIMD Extensions-2 */
XEN_CPUFEATURE(SS,            0*32+27) /*A  CPU self snoop */
XEN_CPUFEATURE(HTT,           0*32+28) /*!A Hyper-Threading Technology */
XEN_CPUFEATURE(TM1,           0*32+29) /*   Thermal Monitor 1 */
XEN_CPUFEATURE(PBE,           0*32+31) /*   Pending Break Enable */

/* Intel-defined CPU features, CPUID level 0x00000001.ecx, word 1 */
XEN_CPUFEATURE(SSE3,          1*32+ 0) /*A  Streaming SIMD Extensions-3 */
XEN_CPUFEATURE(PCLMULQDQ,     1*32+ 1) /*A  Carry-less multiplication */
XEN_CPUFEATURE(DTES64,        1*32+ 2) /*   64-bit Debug Store */
XEN_CPUFEATURE(MONITOR,       1*32+ 3) /*   Monitor/Mwait support */
XEN_CPUFEATURE(DSCPL,         1*32+ 4) /*   CPL Qualified Debug Store */
XEN_CPUFEATURE(VMX,           1*32+ 5) /*h  Virtual Machine Extensions */
XEN_CPUFEATURE(SMX,           1*32+ 6) /*   Safer Mode Extensions */
XEN_CPUFEATURE(EIST,          1*32+ 7) /*   Enhanced SpeedStep */
XEN_CPUFEATURE(TM2,           1*32+ 8) /*   Thermal Monitor 2 */
XEN_CPUFEATURE(SSSE3,         1*32+ 9) /*A  Supplemental Streaming SIMD Extensions-3 */
XEN_CPUFEATURE(SDBG,          1*32+11) /*   IA32_DEBUG_INTERFACE MSR for silicon debugging support */
XEN_CPUFEATURE(FMA,           1*32+12) /*A  Fused Multiply Add */
XEN_CPUFEATURE(CX16,          1*32+13) /*A  CMPXCHG16B */
XEN_CPUFEATURE(XTPR,          1*32+14) /*   Send Task Priority Messages */
XEN_CPUFEATURE(PDCM,          1*32+15) /*   Perf/Debug Capability MSR */
XEN_CPUFEATURE(PCID,          1*32+17) /*H  Process Context ID */
XEN_CPUFEATURE(DCA,           1*32+18) /*   Direct Cache Access */
XEN_CPUFEATURE(SSE4_1,        1*32+19) /*A  Streaming SIMD Extensions 4.1 */
XEN_CPUFEATURE(SSE4_2,        1*32+20) /*A  Streaming SIMD Extensions 4.2 */
XEN_CPUFEATURE(X2APIC,        1*32+21) /*!S Extended xAPIC */
XEN_CPUFEATURE(MOVBE,         1*32+22) /*A  movbe instruction */
XEN_CPUFEATURE(POPCNT,        1*32+23) /*A  POPCNT instruction */
XEN_CPUFEATURE(TSC_DEADLINE,  1*32+24) /*S  TSC Deadline Timer */
XEN_CPUFEATURE(AESNI,         1*32+25) /*A  AES instructions */
XEN_CPUFEATURE(XSAVE,         1*32+26) /*A  XSAVE/XRSTOR/XSETBV/XGETBV */
XEN_CPUFEATURE(OSXSAVE,       1*32+27) /*!  OSXSAVE */
XEN_CPUFEATURE(AVX,           1*32+28) /*A  Advanced Vector Extensions */
XEN_CPUFEATURE(F16C,          1*32+29) /*A  Half-precision convert instruction */
XEN_CPUFEATURE(RDRAND,        1*32+30) /*!A Digital Random Number Generator */
XEN_CPUFEATURE(HYPERVISOR,    1*32+31) /*!A Running under some hypervisor */

/* AMD-defined CPU features, CPUID level 0x80000001.edx, word 2 */
XEN_CPUFEATURE(SYSCALL,       2*32+11) /*A  SYSCALL/SYSRET */
XEN_CPUFEATURE(NX,            2*32+20) /*A  Execute Disable */
XEN_CPUFEATURE(MMXEXT,        2*32+22) /*A  AMD MMX extensions */
XEN_CPUFEATURE(FFXSR,         2*32+25) /*A  FFXSR instruction optimizations */
XEN_CPUFEATURE(PAGE1GB,       2*32+26) /*H  1Gb large page support */
XEN_CPUFEATURE(RDTSCP,        2*32+27) /*A  RDTSCP */
XEN_CPUFEATURE(LM,            2*32+29) /*A  Long Mode (x86-64) */
XEN_CPUFEATURE(3DNOWEXT,      2*32+30) /*A  AMD 3DNow! extensions */
XEN_CPUFEATURE(3DNOW,         2*32+31) /*A  3DNow! */

/* AMD-defined CPU features, CPUID level 0x80000001.ecx, word 3 */
XEN_CPUFEATURE(LAHF_LM,       3*32+ 0) /*A  LAHF/SAHF in long mode */
XEN_CPUFEATURE(CMP_LEGACY,    3*32+ 1) /*!A If yes HyperThreading not valid */
XEN_CPUFEATURE(SVM,           3*32+ 2) /*h  Secure virtual machine */
XEN_CPUFEATURE(EXTAPIC,       3*32+ 3) /*   Extended APIC space */
XEN_CPUFEATURE(CR8_LEGACY,    3*32+ 4) /*S  CR8 in 32-bit mode */
XEN_CPUFEATURE(ABM,           3*32+ 5) /*A  Advanced bit manipulation */
XEN_CPUFEATURE(SSE4A,         3*32+ 6) /*A  SSE-4A */
XEN_CPUFEATURE(MISALIGNSSE,   3*32+ 7) /*A  Misaligned SSE mode */
XEN_CPUFEATURE(3DNOWPREFETCH, 3*32+ 8) /*A  3DNow prefetch instructions */
XEN_CPUFEATURE(OSVW,          3*32+ 9) /*   OS Visible Workaround */
XEN_CPUFEATURE(IBS,           3*32+10) /*   Instruction Based Sampling */
XEN_CPUFEATURE(XOP,           3*32+11) /*A  extended AVX instructions */
XEN_CPUFEATURE(SKINIT,        3*32+12) /*   SKINIT/STGI instructions */
XEN_CPUFEATURE(WDT,           3*32+13) /*   Watchdog timer */
XEN_CPUFEATURE(LWP,           3*32+15) /*   Light Weight Profiling */
XEN_CPUFEATURE(FMA4,          3*32+16) /*A  4 operands MAC instructions */
XEN_CPUFEATURE(NODEID_MSR,    3*32+19) /*   NodeId MSR */
XEN_CPUFEATURE(TBM,           3*32+21) /*A  trailing bit manipulations */
XEN_CPUFEATURE(TOPOEXT,       3*32+22) /*   topology extensions CPUID leafs */
XEN_CPUFEATURE(DBEXT,         3*32+26) /*A  data breakpoint extension */
XEN_CPUFEATURE(MONITORX,      3*32+29) /*   MONITOR extension (MONITORX/MWAITX) */
XEN_CPUFEATURE(ADDR_MSK_EXT,  3*32+30) /*A  Address Mask Extentions */

/* Intel-defined CPU features, CPUID level 0x0000000D:1.eax, word 4 */
XEN_CPUFEATURE(XSAVEOPT,      4*32+ 0) /*A  XSAVEOPT instruction */
XEN_CPUFEATURE(XSAVEC,        4*32+ 1) /*A  XSAVEC/XRSTORC instructions */
XEN_CPUFEATURE(XGETBV1,       4*32+ 2) /*A  XGETBV with %ecx=1 */
XEN_CPUFEATURE(XSAVES,        4*32+ 3) /*S  XSAVES/XRSTORS instructions */
XEN_CPUFEATURE(XFD,           4*32+ 4) /*   MSR_XFD{,_ERR} (eXtended Feature Disable) */

/* Intel-defined CPU features, CPUID level 0x00000007:0.ebx, word 5 */
XEN_CPUFEATURE(FSGSBASE,      5*32+ 0) /*A  {RD,WR}{FS,GS}BASE instructions */
XEN_CPUFEATURE(TSC_ADJUST,    5*32+ 1) /*S  TSC_ADJUST MSR available */
XEN_CPUFEATURE(SGX,           5*32+ 2) /*   Software Guard extensions */
XEN_CPUFEATURE(BMI1,          5*32+ 3) /*A  1st bit manipulation extensions */
XEN_CPUFEATURE(HLE,           5*32+ 4) /*!a Hardware Lock Elision */
XEN_CPUFEATURE(AVX2,          5*32+ 5) /*A  AVX2 instructions */
XEN_CPUFEATURE(FDP_EXCP_ONLY, 5*32+ 6) /*!  x87 FDP only updated on exception. */
XEN_CPUFEATURE(SMEP,          5*32+ 7) /*S  Supervisor Mode Execution Protection */
XEN_CPUFEATURE(BMI2,          5*32+ 8) /*A  2nd bit manipulation extensions */
XEN_CPUFEATURE(ERMS,          5*32+ 9) /*A  Enhanced REP MOVSB/STOSB */
XEN_CPUFEATURE(INVPCID,       5*32+10) /*H  Invalidate Process Context ID */
XEN_CPUFEATURE(RTM,           5*32+11) /*!A Restricted Transactional Memory */
XEN_CPUFEATURE(PQM,           5*32+12) /*   Platform QoS Monitoring */
XEN_CPUFEATURE(NO_FPU_SEL,    5*32+13) /*!  FPU CS/DS stored as zero */
XEN_CPUFEATURE(MPX,           5*32+14) /*s  Memory Protection Extensions */
XEN_CPUFEATURE(PQE,           5*32+15) /*   Platform QoS Enforcement */
XEN_CPUFEATURE(AVX512F,       5*32+16) /*A  AVX-512 Foundation Instructions */
XEN_CPUFEATURE(AVX512DQ,      5*32+17) /*A  AVX-512 Doubleword & Quadword Instrs */
XEN_CPUFEATURE(RDSEED,        5*32+18) /*A  RDSEED instruction */
XEN_CPUFEATURE(ADX,           5*32+19) /*A  ADCX, ADOX instructions */
XEN_CPUFEATURE(SMAP,          5*32+20) /*S  Supervisor Mode Access Prevention */
XEN_CPUFEATURE(AVX512_IFMA,   5*32+21) /*A  AVX-512 Integer Fused Multiply Add */
XEN_CPUFEATURE(CLFLUSHOPT,    5*32+23) /*A  CLFLUSHOPT instruction */
XEN_CPUFEATURE(CLWB,          5*32+24) /*!A CLWB instruction */
XEN_CPUFEATURE(PROC_TRACE,    5*32+25) /*   Processor Trace */
XEN_CPUFEATURE(AVX512PF,      5*32+26) /*   Xeon Phi AVX-512 Prefetch Instructions */
XEN_CPUFEATURE(AVX512ER,      5*32+27) /*   Xeon Phi AVX-512 Exponent & Reciprocal Instrs */
XEN_CPUFEATURE(AVX512CD,      5*32+28) /*A  AVX-512 Conflict Detection Instrs */
XEN_CPUFEATURE(SHA,           5*32+29) /*A  SHA1 & SHA256 instructions */
XEN_CPUFEATURE(AVX512BW,      5*32+30) /*A  AVX-512 Byte and Word Instructions */
XEN_CPUFEATURE(AVX512VL,      5*32+31) /*A  AVX-512 Vector Length Extensions */

/* Intel-defined CPU features, CPUID level 0x00000007:0.ecx, word 6 */
XEN_CPUFEATURE(PREFETCHWT1,   6*32+ 0) /*A  PREFETCHWT1 instruction */
XEN_CPUFEATURE(AVX512_VBMI,   6*32+ 1) /*A  AVX-512 Vector Byte Manipulation Instrs */
XEN_CPUFEATURE(UMIP,          6*32+ 2) /*S  User Mode Instruction Prevention */
XEN_CPUFEATURE(PKU,           6*32+ 3) /*H  Protection Keys for Userspace */
XEN_CPUFEATURE(OSPKE,         6*32+ 4) /*!  OS Protection Keys Enable */
XEN_CPUFEATURE(WAITPKG,       6*32+ 5) /*   UMONITOR/UMWAIT/TPAUSE */
XEN_CPUFEATURE(AVX512_VBMI2,  6*32+ 6) /*A  Additional AVX-512 Vector Byte Manipulation Instrs */
XEN_CPUFEATURE(CET_SS,        6*32+ 7) /*   CET - Shadow Stacks */
XEN_CPUFEATURE(GFNI,          6*32+ 8) /*A  Galois Field Instrs */
XEN_CPUFEATURE(VAES,          6*32+ 9) /*A  Vector AES Instrs */
XEN_CPUFEATURE(VPCLMULQDQ,    6*32+10) /*A  Vector Carry-less Multiplication Instrs */
XEN_CPUFEATURE(AVX512_VNNI,   6*32+11) /*A  Vector Neural Network Instrs */
XEN_CPUFEATURE(AVX512_BITALG, 6*32+12) /*A  Support for VPOPCNT[B,W] and VPSHUFBITQMB */
XEN_CPUFEATURE(TME,           6*32+13) /*   Total Memory Encryption */
XEN_CPUFEATURE(AVX512_VPOPCNTDQ, 6*32+14) /*A  POPCNT for vectors of DW/QW */
XEN_CPUFEATURE(LA57,          6*32+16) /*   5-level paging (57-bit linear address) */
XEN_CPUFEATURE(RDPID,         6*32+22) /*A  RDPID instruction */
XEN_CPUFEATURE(BLD,           6*32+24) /*   BusLock Detect (#DB trap) support */
XEN_CPUFEATURE(CLDEMOTE,      6*32+25) /*A  CLDEMOTE instruction */
XEN_CPUFEATURE(MOVDIRI,       6*32+27) /*a  MOVDIRI instruction */
XEN_CPUFEATURE(MOVDIR64B,     6*32+28) /*a  MOVDIR64B instruction */
XEN_CPUFEATURE(ENQCMD,        6*32+29) /*   ENQCMD{,S} instructions */
XEN_CPUFEATURE(SGX_LC,        6*32+30) /*   SGX Launch Configuration */
XEN_CPUFEATURE(PKS,           6*32+31) /*H  Protection Key for Supervisor */

/* AMD-defined CPU features, CPUID level 0x80000007.edx, word 7 */
XEN_CPUFEATURE(HW_PSTATE,     7*32+ 7) /*   Hardware Pstates */
XEN_CPUFEATURE(ITSC,          7*32+ 8) /*a  Invariant TSC */
XEN_CPUFEATURE(CPB,           7*32+ 9) /*   Core Performance Boost (Turbo) */
XEN_CPUFEATURE(EFRO,          7*32+10) /*   APERF/MPERF Read Only interface */

/* AMD-defined CPU features, CPUID level 0x80000008.ebx, word 8 */
XEN_CPUFEATURE(CLZERO,        8*32+ 0) /*A  CLZERO instruction */
XEN_CPUFEATURE(RSTR_FP_ERR_PTRS, 8*32+ 2) /*A  (F)X{SAVE,RSTOR} always saves/restores FPU Error pointers */
XEN_CPUFEATURE(WBNOINVD,      8*32+ 9) /*   WBNOINVD instruction */
XEN_CPUFEATURE(IBPB,          8*32+12) /*A  IBPB support only (no IBRS, used by AMD) */
XEN_CPUFEATURE(IBRS,          8*32+14) /*S  MSR_SPEC_CTRL.IBRS */
XEN_CPUFEATURE(AMD_STIBP,     8*32+15) /*S  MSR_SPEC_CTRL.STIBP */
XEN_CPUFEATURE(IBRS_ALWAYS,   8*32+16) /*S  IBRS preferred always on */
XEN_CPUFEATURE(STIBP_ALWAYS,  8*32+17) /*S  STIBP preferred always on */
XEN_CPUFEATURE(IBRS_FAST,     8*32+18) /*S  IBRS preferred over software options */
XEN_CPUFEATURE(IBRS_SAME_MODE, 8*32+19) /*S  IBRS provides same-mode protection */
XEN_CPUFEATURE(NO_LMSL,       8*32+20) /*S| EFER.LMSLE no longer supported. */
XEN_CPUFEATURE(AMD_PPIN,      8*32+23) /*   Protected Processor Inventory Number */
XEN_CPUFEATURE(AMD_SSBD,      8*32+24) /*S  MSR_SPEC_CTRL.SSBD available */
XEN_CPUFEATURE(VIRT_SSBD,     8*32+25) /*!  MSR_VIRT_SPEC_CTRL.SSBD */
XEN_CPUFEATURE(SSB_NO,        8*32+26) /*A  Hardware not vulnerable to SSB */
XEN_CPUFEATURE(PSFD,          8*32+28) /*S  MSR_SPEC_CTRL.PSFD */
XEN_CPUFEATURE(BTC_NO,        8*32+29) /*A  Hardware not vulnerable to Branch Type Confusion */
XEN_CPUFEATURE(IBPB_RET,      8*32+30) /*A  IBPB clears RSB/RAS too. */

/* Intel-defined CPU features, CPUID level 0x00000007:0.edx, word 9 */
XEN_CPUFEATURE(SGX_KEYS,      9*32+ 1) /*   SGX Attestation Service */
XEN_CPUFEATURE(AVX512_4VNNIW, 9*32+ 2) /*   Xeon Phi AVX512 Neural Network Instructions */
XEN_CPUFEATURE(AVX512_4FMAPS, 9*32+ 3) /*   Xeon Phi AVX512 Multiply Accumulation Single Precision */
XEN_CPUFEATURE(FSRM,          9*32+ 4) /*A  Fast Short REP MOVS */
XEN_CPUFEATURE(UINTR,         9*32+ 5) /*   User-mode Interrupts */
XEN_CPUFEATURE(AVX512_VP2INTERSECT, 9*32+8) /*a  VP2INTERSECT{D,Q} insns */
XEN_CPUFEATURE(SRBDS_CTRL,    9*32+ 9) /*   MSR_MCU_OPT_CTRL and RNGDS_MITG_DIS. */
XEN_CPUFEATURE(MD_CLEAR,      9*32+10) /*!A| VERW clears microarchitectural buffers */
XEN_CPUFEATURE(RTM_ALWAYS_ABORT, 9*32+11) /*! RTM disabled (but XBEGIN wont fault) */
XEN_CPUFEATURE(TSX_FORCE_ABORT, 9*32+13) /* MSR_TSX_FORCE_ABORT.RTM_ABORT */
XEN_CPUFEATURE(SERIALIZE,     9*32+14) /*A  SERIALIZE insn */
XEN_CPUFEATURE(HYBRID,        9*32+15) /*   Heterogeneous platform */
XEN_CPUFEATURE(TSXLDTRK,      9*32+16) /*a  TSX load tracking suspend/resume insns */
XEN_CPUFEATURE(PCONFIG,       9*32+18) /*   PCONFIG instruction */
XEN_CPUFEATURE(ARCH_LBR,      9*32+19) /*   Architectural Last Branch Record */
XEN_CPUFEATURE(CET_IBT,       9*32+20) /*   CET - Indirect Branch Tracking */
XEN_CPUFEATURE(AMX_BF16,      9*32+22) /*   AMX BFloat16 instruction */
XEN_CPUFEATURE(AVX512_FP16,   9*32+23) /*A  AVX512 FP16 instructions */
XEN_CPUFEATURE(AMX_TILE,      9*32+24) /*   AMX Tile architecture */
XEN_CPUFEATURE(AMX_INT8,      9*32+25) /*   AMX 8-bit integer instructions */
XEN_CPUFEATURE(IBRSB,         9*32+26) /*A  IBRS and IBPB support (used by Intel) */
XEN_CPUFEATURE(STIBP,         9*32+27) /*A  STIBP */
XEN_CPUFEATURE(L1D_FLUSH,     9*32+28) /*S  MSR_FLUSH_CMD and L1D flush. */
XEN_CPUFEATURE(ARCH_CAPS,     9*32+29) /*!A IA32_ARCH_CAPABILITIES MSR */
XEN_CPUFEATURE(CORE_CAPS,     9*32+30) /*   IA32_CORE_CAPABILITIES MSR */
XEN_CPUFEATURE(SSBD,          9*32+31) /*A  MSR_SPEC_CTRL.SSBD available */

/* Intel-defined CPU features, CPUID level 0x00000007:1.eax, word 10 */
XEN_CPUFEATURE(SHA512,       10*32+ 0) /*A  SHA512 Instructions */
XEN_CPUFEATURE(SM3,          10*32+ 1) /*A  SM3 Instructions */
XEN_CPUFEATURE(SM4,          10*32+ 2) /*A  SM4 Instructions */
XEN_CPUFEATURE(AVX_VNNI,     10*32+ 4) /*A  AVX-VNNI Instructions */
XEN_CPUFEATURE(AVX512_BF16,  10*32+ 5) /*A  AVX512 BFloat16 Instructions */
XEN_CPUFEATURE(CMPCCXADD,    10*32+ 7) /*a  CMPccXADD Instructions */
XEN_CPUFEATURE(FZRM,         10*32+10) /*A  Fast Zero-length REP MOVSB */
XEN_CPUFEATURE(FSRS,         10*32+11) /*A  Fast Short REP STOSB */
XEN_CPUFEATURE(FSRCS,        10*32+12) /*A  Fast Short REP CMPSB/SCASB */
XEN_CPUFEATURE(FRED,         10*32+17) /*   Fast Return and Event Delivery */
XEN_CPUFEATURE(WRMSRNS,      10*32+19) /*S  WRMSR Non-Serialising */
XEN_CPUFEATURE(AMX_FP16,     10*32+21) /*   AMX FP16 instruction */
XEN_CPUFEATURE(AVX_IFMA,     10*32+23) /*A  AVX-IFMA Instructions */

/* AMD-defined CPU features, CPUID level 0x80000021.eax, word 11 */
XEN_CPUFEATURE(NO_NEST_BP,         11*32+ 0) /*A  No Nested Data Breakpoints */
XEN_CPUFEATURE(FS_GS_NS,           11*32+ 1) /*S| FS/GS base MSRs non-serialising */
XEN_CPUFEATURE(LFENCE_DISPATCH,    11*32+ 2) /*A  LFENCE always serializing */
XEN_CPUFEATURE(VERW_CLEAR,         11*32+ 5) /*!A| VERW clears microarchitectural buffers */
XEN_CPUFEATURE(NSCB,               11*32+ 6) /*A  Null Selector Clears Base (and limit too) */
XEN_CPUFEATURE(AUTO_IBRS,          11*32+ 8) /*S  Automatic IBRS */
XEN_CPUFEATURE(AMD_FSRS,           11*32+10) /*A  Fast Short REP STOSB */
XEN_CPUFEATURE(AMD_FSRC,           11*32+11) /*A  Fast Short REP CMPSB */
XEN_CPUFEATURE(CPUID_USER_DIS,     11*32+17) /*   CPUID disable for CPL > 0 software */
XEN_CPUFEATURE(EPSF,               11*32+18) /*A  Enhanced Predictive Store Forwarding */
XEN_CPUFEATURE(FSRSC,              11*32+19) /*A  Fast Short REP SCASB */
XEN_CPUFEATURE(AMD_PREFETCHI,      11*32+20) /*A  PREFETCHIT{0,1} Instructions */
XEN_CPUFEATURE(SBPB,               11*32+27) /*A  Selective Branch Predictor Barrier */
XEN_CPUFEATURE(IBPB_BRTYPE,        11*32+28) /*A  IBPB flushes Branch Type predictions too */
XEN_CPUFEATURE(SRSO_NO,            11*32+29) /*A  Hardware not vulnerable to Speculative Return Stack Overflow */
XEN_CPUFEATURE(SRSO_US_NO,         11*32+30) /*A! Hardware not vulnerable to SRSO across the User/Supervisor boundary */
XEN_CPUFEATURE(SRSO_MSR_FIX,       11*32+31) /*   MSR_BP_CFG.BP_SPEC_REDUCE available */

/* Intel-defined CPU features, CPUID level 0x00000007:1.ebx, word 12 */
XEN_CPUFEATURE(INTEL_PPIN,         12*32+ 0) /*   Protected Processor Inventory Number */

/* Intel-defined CPU features, CPUID level 0x00000007:2.edx, word 13 */
XEN_CPUFEATURE(INTEL_PSFD,         13*32+ 0) /*A  MSR_SPEC_CTRL.PSFD */
XEN_CPUFEATURE(IPRED_CTRL,         13*32+ 1) /*S  MSR_SPEC_CTRL.IPRED_DIS_* */
XEN_CPUFEATURE(RRSBA_CTRL,         13*32+ 2) /*S  MSR_SPEC_CTRL.RRSBA_DIS_* */
XEN_CPUFEATURE(DDP_CTRL,           13*32+ 3) /*   MSR_SPEC_CTRL.DDP_DIS_U */
XEN_CPUFEATURE(BHI_CTRL,           13*32+ 4) /*S  MSR_SPEC_CTRL.BHI_DIS_S */
XEN_CPUFEATURE(MCDT_NO,            13*32+ 5) /*A  MCDT_NO */

/* Intel-defined CPU features, CPUID level 0x00000007:1.ecx, word 14 */

/* Intel-defined CPU features, CPUID level 0x00000007:1.edx, word 15 */
XEN_CPUFEATURE(AVX_VNNI_INT8,      15*32+ 4) /*A  AVX-VNNI-INT8 Instructions */
XEN_CPUFEATURE(AVX_NE_CONVERT,     15*32+ 5) /*A  AVX-NE-CONVERT Instructions */
XEN_CPUFEATURE(AMX_COMPLEX,        15*32+ 8) /*   AMX Complex Instructions */
XEN_CPUFEATURE(AVX_VNNI_INT16,     15*32+10) /*A  AVX-VNNI-INT16 Instructions */
XEN_CPUFEATURE(PREFETCHI,          15*32+14) /*A  PREFETCHIT{0,1} Instructions */
XEN_CPUFEATURE(CET_SSS,            15*32+18) /*   CET Supervisor Shadow Stacks safe to use */

/* Intel-defined CPU features, MSR_ARCH_CAPS 0x10a.eax, word 16 */
XEN_CPUFEATURE(RDCL_NO,            16*32+ 0) /*A  No Rogue Data Cache Load (Meltdown) */
XEN_CPUFEATURE(EIBRS,              16*32+ 1) /*A  Enhanced IBRS */
XEN_CPUFEATURE(RSBA,               16*32+ 2) /*!  RSB Alternative (Retpoline not safe) */
XEN_CPUFEATURE(SKIP_L1DFL,         16*32+ 3) /*   Don't need to flush L1D on VMEntry */
XEN_CPUFEATURE(INTEL_SSB_NO,       16*32+ 4) /*A  No Speculative Store Bypass */
XEN_CPUFEATURE(MDS_NO,             16*32+ 5) /*A  No Microarchitectural Data Sampling */
XEN_CPUFEATURE(IF_PSCHANGE_MC_NO,  16*32+ 6) /*A  No Instruction fetch #MC */
XEN_CPUFEATURE(TSX_CTRL,           16*32+ 7) /*   MSR_TSX_CTRL */
XEN_CPUFEATURE(TAA_NO,             16*32+ 8) /*A  No TSX Async Abort */
XEN_CPUFEATURE(MCU_CTRL,           16*32+ 9) /*   MSR_MCU_CTRL */
XEN_CPUFEATURE(MISC_PKG_CTRL,      16*32+10) /*   MSR_MISC_PKG_CTRL */
XEN_CPUFEATURE(ENERGY_FILTERING,   16*32+11) /*   MSR_MISC_PKG_CTRL.ENERGY_FILTERING */
XEN_CPUFEATURE(DOITM,              16*32+12) /*   Data Operand Invariant Timing Mode */
XEN_CPUFEATURE(SBDR_SSDP_NO,       16*32+13) /*A  No Shared Buffer Data Read or Sideband Stale Data Propagation */
XEN_CPUFEATURE(FBSDP_NO,           16*32+14) /*A  No Fill Buffer Stale Data Propagation */
XEN_CPUFEATURE(PSDP_NO,            16*32+15) /*A  No Primary Stale Data Propagation */
XEN_CPUFEATURE(FB_CLEAR,           16*32+17) /*!A| Fill Buffers cleared by VERW */
XEN_CPUFEATURE(FB_CLEAR_CTRL,      16*32+18) /*   MSR_OPT_CPU_CTRL.FB_CLEAR_DIS */
XEN_CPUFEATURE(RRSBA,              16*32+19) /*!  Restricted RSB Alternative */
XEN_CPUFEATURE(BHI_NO,             16*32+20) /*A  No Branch History Injection  */
XEN_CPUFEATURE(XAPIC_STATUS,       16*32+21) /*   MSR_XAPIC_DISABLE_STATUS */
XEN_CPUFEATURE(OVRCLK_STATUS,      16*32+23) /*   MSR_OVERCLOCKING_STATUS */
XEN_CPUFEATURE(PBRSB_NO,           16*32+24) /*A  No Post-Barrier RSB predictions */
XEN_CPUFEATURE(GDS_CTRL,           16*32+25) /*   MCU_OPT_CTRL.GDS_MIT_{DIS,LOCK} */
XEN_CPUFEATURE(GDS_NO,             16*32+26) /*A  No Gather Data Sampling */
XEN_CPUFEATURE(RFDS_NO,            16*32+27) /*A  No Register File Data Sampling */
XEN_CPUFEATURE(RFDS_CLEAR,         16*32+28) /*!A| Register File(s) cleared by VERW */

/* Intel-defined CPU features, MSR_ARCH_CAPS 0x10a.edx, word 17 (express in terms of word 16) */
XEN_CPUFEATURE(PB_OPT_CTRL,        16*32+32) /*   MSR_PB_OPT_CTRL.IBPB_ALT */
XEN_CPUFEATURE(ITS_NO,             16*32+62) /*!A No Indirect Target Selection */

/* AMD-defined CPU features, CPUID level 0x80000021.ecx, word 18 */
XEN_CPUFEATURE(TSA_SQ_NO,          18*32+ 1) /*A  No Store Queue Transitive Scheduler Attacks */
XEN_CPUFEATURE(TSA_L1_NO,          18*32+ 2) /*A  No L1D Transitive Scheduler Attacks */

#endif /* XEN_CPUFEATURE */

/* Clean up from a default include.  Close the enum (for C). */
#ifdef XEN_CPUFEATURESET_DEFAULT_INCLUDE
#undef XEN_CPUFEATURESET_DEFAULT_INCLUDE
#undef XEN_CPUFEATURE
};

#endif /* XEN_CPUFEATURESET_DEFAULT_INCLUDE */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
