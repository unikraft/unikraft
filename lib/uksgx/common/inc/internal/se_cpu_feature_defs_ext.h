/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef SE_CPU_FEATURE_DEFS_EXT_H
#define SE_CPU_FEATURE_DEFS_EXT_H

typedef enum {
    c_feature_none = 0,
    c_feature_generic_ia32 = 1,     /* 0x00000001 */
    c_feature_x87 = 2,              /* 0x00000002 */
    c_feature_cmov = 3,             /* 0x00000004 */
    c_feature_mmx = 4,              /* 0x00000008 */
    c_feature_fxsave = 5,           /* 0x00000010 */
    c_feature_sse = 6,              /* 0x00000020 */
    c_feature_sse2 = 7,             /* 0x00000040 */
    c_feature_sse3 = 8,             /* 0x00000080 */
    c_feature_ssse3 = 9,            /* 0x00000100 */
    c_feature_sse4_1 = 10,          /* 0x00000200 */
    c_feature_sse4_2 = 11,          /* 0x00000400 */
    c_feature_movbe = 12,           /* 0x00000800 */
    c_feature_popcnt = 13,          /* 0x00001000 */
    c_feature_pclmulqdq = 14,       /* 0x00002000 */
    c_feature_aes = 15,             /* 0x00004000 */

    /* 16-bit floating-point conversions instructions */
    c_feature_f16c = 16,            /* 0x00008000 */
    /* AVX instructions - SNB */
    c_feature_avx1 = 17,            /* 0x00010000 */
    /* RDRND (read random value) instruction */
    c_feature_rdrnd = 18,           /* 0x00020000 */
    /* FMA, may need more precise name - HSW */
    c_feature_fma = 19,             /* 0x00040000 */
    /* two groups of advanced bit manipulation extensions */
    c_feature_bmi = 20,             /* 0x00080000 */
    /* LZCNT (leading zeroes count) */
    c_feature_lzcnt = 21,           /* 0x00100000 */
    /* HLE (hardware lock elision) */
    c_feature_hle = 22,             /* 0x00200000 */
    /* RTM (restricted transactional memory) */
    c_feature_rtm = 23,             /* 0x00400000 */
    /* AVX2 instructions - HSW */
    c_feature_avx2 = 24,            /* 0x00800000 */
    /* AVX512DQ - SKX 512-bit dword/qword vector instructions */
    c_feature_avx512dq = 25,        /* 0x01000000 */
    /* Unused, remained from KNF */
    c_feature_ptwrite = 26,         /* 0x02000000 */
    /* KNC new instructions */
    c_feature_kncni = 27,           /* 0x04000000 */
    /* AVX-512 foundation instructions - KNL and SKX */
    c_feature_avx512f = 28,         /* 0x08000000 */
    /* uint add with OF or CF flags (ADOX, ADCX) - BDW */
    c_feature_adx = 29,             /* 0x10000000 */
    /* Enhanced non-deterministic rand generator - BDW */
    c_feature_rdseed = 30,          /* 0x20000000 */
    /* AVX512IFMA52:  vpmadd52huq and vpmadd52luq. */
    c_feature_avx512ifma52 = 31,    /* 0x40000000 */
    /* Full inorder (like Silverthorne) processor */
    c_soft_feature_proc_full_inorder = 32,   /* 0x80000000 */
    /* AVX-512 exponential and reciprocal instructions - KNL */
    c_feature_tvx_trans = 33,       /* 0x100000000 */
    /* AVX-512 gather/scatter prefetch instructions - KNL */
    c_feature_tvx_prefetch = 34,    /* 0x200000000 */
    /* AVX-512 conflict detection instructions - KNL */
    c_feature_vconflict = 35,       /* 0x400000000 */
    /* Secure Hash Algorithm instructions (SHA) */
    c_feature_sha = 36,             /* 0x800000000 */
    /* Memory Protection Extensions (MPX) */
    c_feature_mpx = 37,             /* 0x1000000000 */
    /* AVX512BW - SKX 512-bit byte/word vector instructions */
    c_feature_avx512bw = 38,        /* 0x2000000000 */
    /* AVX512VL - adds 128/256-bit vector support of other AVX512 instructions. */
    c_feature_avx512vl = 39,        /* 0x4000000000 */
    /* AVX512VBMI:  vpermb, vpermi2b, vpermt2b and vpmultishiftqb. */
    c_feature_avx512vbmi = 40,      /* 0x8000000000 */
    /* AVX512_4FMAPS: Single Precision FMA for multivector(4 vector) operand. */
    c_feature_avx512_4fmaps = 41,   /* 0x10000000000 */
    /* AVX512_4VNNIW: Vector Neural Network Instructions for
    *  multivector(4 vector) operand with word elements. */
    c_feature_avx512_4vnniw = 42,   /* 0x20000000000 */
    /* AVX512_VPOPCNTDQ: 512-bit vector POPCNT. */
    c_feature_avx512_vpopcntdq = 43,   /* 0x40000000000 */
    /* AVX512_BITALG: vector bit algebra in AVX512. */
    c_feature_avx512_bitalg = 44,   /* 0x80000000000 */
    /* AVX512_VBMI2: additional byte, word, dword and qword capabilities */
    c_feature_avx512vbmi2 = 45,     /* 0x100000000000 */
    /* GFNI: Galois Field New Instructions. */
    c_feature_gfni = 46,            /* 0x200000000000 */
    /* VAES: vector AES instructions */
    c_feature_vaes = 47,            /* 0x400000000000 */
    /* VPCLMULQDQ: vector PCLMULQDQ instructions. */
    c_feature_vpclmulqdq = 48,      /* 0x800000000000 */
    /* AVX512_VNNI: vector Neural Network Instructions. */
    c_feature_avx512vnni = 49,      /* 0x1000000000000 */
    /* CLWB: Cache Line Write Back. */
    c_feature_clwb = 50,            /* 0x2000000000000 */
    /* RDPID: Read Processor ID. */
    c_feature_rdpid = 51,           /* 0x4000000000000 */
    c_feature_ibt = 52,             /* 0x8000000000000 */
    c_feature_shstk = 53,
    c_feature_sgx = 54,
    c_feature_wbnoinvd = 55,
    c_feature_pconfig = 56,
    c_feature_end
} FeatureId;

#define CPU_ATOM1          0x1c
#define CPU_ATOM2          0x26
#define CPU_ATOM3          0x27



#define CPU_FAMILY(x)     (((x) >> 8) & 0xf)
#define CPU_MODEL(x)      (((x) >> 4) & 0xf)
#define CPU_STEPPING(x)   (((x) >> 0) & 0xf)

#define CPU_HAS_MMX(x)    (((x) & 0x00800000) != 0)
#define CPU_HAS_FXSAVE(x) (((x) & (1 << 24)) != 0)
#define CPU_HAS_SSE(x)    (((x) & (1 << 25)) != 0)
#define CPU_HAS_SSE2(x)   (((x) & (1 << 26)) != 0)
#define CPU_HAS_PNI(x)    (((x) & (1 << 0)) != 0)
#define CPU_HAS_MNI(x)    (((x) & (1 << 9)) != 0)
#define CPU_HAS_SNI(x)    (((x) & (1 << 19)) != 0)
#define CPU_HAS_MOVBE(x)  (((x) & (1 << 22)) != 0)
#define CPU_HAS_SSE4_2(x) (((x) & (1 << 20)) != 0)
#define CPU_HAS_POPCNT(x) (((x) & (1 << 23)) != 0)
#define CPU_HAS_PCLMULQDQ(x) (((x) & (1 <<  1)) != 0)
#define CPU_HAS_AES(x)       (((x) & (1 << 25)) != 0)
#define CPU_HAS_XSAVE(x)  (((x) & (1 << 27)) != 0)
#define CPU_HAS_AVX(x)    (((x) & (1 << 28)) != 0)
#define XFEATURE_ENABLED_AVX(x) \
                          (((x) & 0x06) == 0x06)
#define XFEATURE_ENABLED_AVX3(x) \
                          (((x) & 0xE0) == 0xE0)
#define CPU_HAS_F16C(x)   (((x) & (1 << 29)) != 0)
#define CPU_HAS_RDRAND(x) (((x) & (1 << 30)) != 0)
#define CPU_HAS_IVB(x)    (CPU_HAS_F16C(x) && CPU_HAS_RDRAND(x))
#define CPU_HAS_IVB_NORDRAND(x)  (CPU_HAS_F16C(x))
#define CPU_HAS_AVX2(x)                 (((x) & (1 << 5)) != 0)
#define CPU_HAS_HLE(x)                  (((x) & (1 << 4)) != 0)
#define CPU_HAS_RTM(x)                  (((x) & (1 << 11)) != 0)
#define CPU_HAS_BMI(x)                  (((x) & (1 << 3)) != 0 && \
                                         ((x) & (1 << 8)) != 0)
#define CPU_HAS_LZCNT(x)                (((x) & (1 << 5)) != 0)
#define CPU_HAS_FMA(x)                  (((x) & (1 << 12)) != 0)
#define CPU_HAS_HSW(cpuid7_ebx, ecpuid1_ecx, cpuid1_ecx) \
        (CPU_HAS_AVX2(cpuid7_ebx) && CPU_HAS_BMI(cpuid7_ebx) && \
         CPU_HAS_LZCNT(ecpuid1_ecx) && CPU_HAS_FMA(cpuid1_ecx) && \
         CPU_HAS_HLE(cpuid7_ebx) && CPU_HAS_RTM(cpuid7_ebx))

#define CPU_HAS_BDW(x)                  (((x) & (1 << 18)) != 0 && \
                                         ((x) & (1 << 19)) != 0)
#define CPU_HAS_KNL(x)                  (((x) & (1 << 26)) != 0 && \
                                         ((x) & (1 << 16)) != 0 && \
                                         ((x) & (1 << 27)) != 0 && \
                                         ((x) & (1 << 28)) != 0)

#define CPU_HAS_SKX(x)                  (((x) & (1 << 16)) != 0 && \
                                         ((x) & (1 << 17)) != 0 && \
                                         ((x) & (1 << 30)) != 0 && \
                                         ((x) & (1 << 31)) != 0)

#define CPU_HAS_SNC(x)                  (((x) & (1 << 6))  != 0 && \
                                         ((x) & (1 << 8))  != 0 && \
                                         ((x) & (1 << 9))  != 0 && \
                                         ((x) & (1 << 10)) != 0 && \
                                         ((x) & (1 << 11)) != 0 && \
                                         ((x) & (1 << 12)) != 0)

#define CPU_HAS_KNH(x)                  (((x) & (1 << 12)) != 0 && \
                                         ((x) & (1 << 14)) != 0)

#define CPU_GENU_VAL      ('G' << 0 | 'e' << 8 | 'n' << 16 | 'u' << 24)
#define CPU_INEI_VAL      ('i' << 0 | 'n' << 8 | 'e' << 16 | 'I' << 24)
#define CPU_NTEL_VAL      ('n' << 0 | 't' << 8 | 'e' << 16 | 'l' << 24)


#define CPU_GENU_VAL      ('G' << 0 | 'e' << 8 | 'n' << 16 | 'u' << 24)
#define CPU_INEI_VAL      ('i' << 0 | 'n' << 8 | 'e' << 16 | 'I' << 24)
#define CPU_NTEL_VAL      ('n' << 0 | 't' << 8 | 'e' << 16 | 'l' << 24)



#define CPU_GENERIC             0x1
#define CPU_PENTIUM             0x2
#define CPU_PENTIUM_PRO         0x4
#define CPU_PENTIUM_MMX         0x8
#define CPU_PENTIUM_II          0x10
#define CPU_PENTIUM_II_FXSV     0x20
#define CPU_PENTIUM_III         0x40
#define CPU_PENTIUM_III_SSE     0x80
#define CPU_PENTIUM_4           0x100
#define CPU_PENTIUM_4_SSE2      0x200
#define CPU_BNI                 0x400
#define CPU_PENTIUM_4_PNI       0x800
#define CPU_MNI                 0x1000
#define CPU_SNI                 0x2000
#define CPU_BNL                 0x4000
#define CPU_NHM                 0x8000
#define CPU_WSM                 0x10000
#define CPU_SNB                 0x20000
#define CPU_IVB                 0x40000
#define CPU_HSW                 0x200000
#define CPU_BDW                 0x800000
#define CPU_KNL                 0x2000000
#define CPU_SKX                 0x4000000
#define CPU_KNH                 0x8000000
#define CPU_SNC                 0x10000000

#define CPU_PENTIUM_FAMILY 5
#define CPU_PPRO_FAMILY    6
#define CPU_WMT_FAMILY     15


//
// The processor is a generic IA32 CPU
//
#define CPU_FEATURE_GENERIC_IA32        0x00000001ULL

//
// Floating point unit is on-chip.
//
#define CPU_FEATURE_FPU                 0x00000002ULL

//
// Conditional mov instructions are supported.
//
#define CPU_FEATURE_CMOV                0x00000004ULL

//
// The processor supports the MMX technology instruction set extensions
// to Intel Architecture.
//
#define CPU_FEATURE_MMX                 0x00000008ULL

//
// The FXSAVE and FXRSTOR instructions are supported for fast
// save and restore of the floating point context.
//
#define CPU_FEATURE_FXSAVE              0x00000010ULL

//
// Indicates the processor supports the Streaming SIMD Extensions Instructions.
//
#define CPU_FEATURE_SSE                 0x00000020ULL

//
// Indicates the processor supports the Streaming SIMD
// Extensions 2 Instructions.
//
#define CPU_FEATURE_SSE2                0x00000040ULL

//
// Indicates the processor supports the Streaming SIMD
// Extensions 3 Instructions. (PNI)
//
#define CPU_FEATURE_SSE3                0x00000080ULL

//
// The processor supports the Supplemental Streaming SIMD Extensions 3
// instructions. (MNI)
//
#define CPU_FEATURE_SSSE3               0x00000100ULL

//
// The processor supports the Streaming SIMD Extensions 4.1 instructions.(SNI)
//
#define CPU_FEATURE_SSE4_1              0x00000200ULL

//
// The processor supports the Streaming SIMD Extensions 4.1 instructions.
// (NNI + STTNI)
//
#define CPU_FEATURE_SSE4_2              0x00000400ULL

//
// The processor supports MOVBE instruction.
//
#define CPU_FEATURE_MOVBE               0x00000800ULL


//
// The processor supports POPCNT instruction.
//
#define CPU_FEATURE_POPCNT              0x00001000ULL


//
// The processor supports PCLMULQDQ instruction.
//
#define CPU_FEATURE_PCLMULQDQ           0x00002000ULL

//
// The processor supports instruction extension for encryption.
//
#define CPU_FEATURE_AES                 0x00004000ULL

//
// The processor supports 16-bit floating-point conversions instructions.
//
#define CPU_FEATURE_F16C                0x00008000ULL

//
// The processor supports AVX instruction extension.
//
#define CPU_FEATURE_AVX                 0x00010000ULL

//
// The processor supports RDRND (read random value) instruction.
//
#define CPU_FEATURE_RDRND               0x00020000ULL

//
// The processor supports FMA instructions.
//
#define CPU_FEATURE_FMA                 0x00040000ULL

//
// The processor supports two groups of advanced bit manipulation extensions. - Haswell introduced, AVX2 related 
//
#define CPU_FEATURE_BMI                 0x00080000ULL

//
// The processor supports LZCNT instruction (counts the number of leading zero
// bits). - Haswell introduced
//
#define CPU_FEATURE_LZCNT               0x00100000ULL

//
// The processor supports HLE extension (hardware lock elision). - Haswell introduced
//
#define CPU_FEATURE_HLE                 0x00200000ULL

//
// The processor supports RTM extension (restricted transactional memory) - Haswell AVX2 related.
//
#define CPU_FEATURE_RTM                 0x00400000ULL

//
// The processor supports AVX2 instruction extension.
//
#define CPU_FEATURE_AVX2                0x00800000ULL

//
// The processor supports AVX512 dword/qword instruction extension. 
//
#define CPU_FEATURE_AVX512DQ            0x01000000ULL

//
// The processor supports the PTWRITE instruction.
//
#define CPU_FEATURE_PTWRITE             0x02000000ULL

//
// KNC instruction set
//
#define CPU_FEATURE_KNCNI               0x04000000ULL

//
// AVX512 foundation instructions
//
#define CPU_FEATURE_AVX512F             0x08000000ULL

//
// The processor supports uint add with OF or CF flags (ADOX, ADCX)
//
#define CPU_FEATURE_ADX                 0x10000000ULL

//
// The processor supports RDSEED instruction.
//
#define CPU_FEATURE_RDSEED              0x20000000ULL

// AVX512IFMA52: vpmadd52huq and vpmadd52luq
#define CPU_FEATURE_AVX512IFMA52        0x40000000ULL

//
// The processor is a full inorder (Silverthorne) processor
// 
#define CPU_FEATURE_FULL_INORDER        0x80000000ULL


// AVX512 exponential and reciprocal instructions
#define CPU_FEATURE_AVX512ER            0x100000000ULL

// AVX512 prefetch instructions
#define CPU_FEATURE_AVX512PF            0x200000000ULL

// AVX-512 conflict detection instructions
#define CPU_FEATURE_AVX512CD            0x400000000ULL

// Secure Hash Algorithm instructions (SHA)
#define CPU_FEATURE_SHA                 0x800000000ULL

// Memory Protection Extensions (MPX)
#define CPU_FEATURE_MPX                 0x1000000000ULL

// AVX512BW - AVX512 byte/word vector instruction set
#define CPU_FEATURE_AVX512BW            0x2000000000ULL

// AVX512VL - 128/256-bit vector support of AVX512 instructions
#define CPU_FEATURE_AVX512VL            0x4000000000ULL

// AVX512VBMI:  vpermb, vpermi2b, vpermt2b and vpmultishiftqb
#define CPU_FEATURE_AVX512VBMI          0x8000000000ULL

// AVX512_4FMAPS: Single Precision FMA for multivector(4 vector) operand.
#define CPU_FEATURE_AVX512_4FMAPS       0x10000000000ULL

// AVX512_4VNNIW: Vector Neural Network Instructions for multivector(4 vector) operand with word elements.
#define CPU_FEATURE_AVX512_4VNNIW       0x20000000000ULL

// AVX512_VPOPCNTDQ: 512-bit vector POPCNT instruction.
#define CPU_FEATURE_AVX512_VPOPCNTDQ    0x40000000000ULL

// AVX512_BITALG: vector bit algebra in AVX512
#define CPU_FEATURE_AVX512_BITALG       0x80000000000ULL

// AVX512_VBMI2: additional byte, word, dword and qword capabilities
#define CPU_FEATURE_AVX512_VBMI2        0x100000000000ULL

// GFNI: Galois Field New Instructions.
#define CPU_FEATURE_GFNI                0x200000000000ULL

// VAES: vector AES instructions
#define CPU_FEATURE_VAES                0x400000000000ULL

// VPCLMULQDQ: Vector CLMUL instruction set.
#define CPU_FEATURE_VPCLMULQDQ          0x800000000000ULL

// AVX512_VNNI: vector Neural Network Instructions.
#define CPU_FEATURE_AVX512_VNNI         0x1000000000000ULL

// CLWB: Cache Line Write Back
#define CPU_FEATURE_CLWB                0x2000000000000ULL

// RDPID: Read Processor ID.
#define CPU_FEATURE_RDPID               0x4000000000000ULL

// IBT - Indirect branch tracking
#define CPU_FEATURE_IBT                 0x8000000000000ULL

// Shadow stack
#define CPU_FEATURE_SHSTK               0x10000000000000ULL

// Intel Software Guard Extensions
#define CPU_FEATURE_SGX                 0x20000000000000ULL

// Write back and do not invalidate cache
#define CPU_FEATURE_WBNOINVD            0x40000000000000ULL

// Platform configuration - 1 << 55
#define CPU_FEATURE_PCONFIG             0x80000000000000ULL


// Reserved feature bits
#define RESERVED_CPU_FEATURE_BIT        (~(0x100000000000000ULL - 1))

// Incompatible bits which we should unset in trts
#define INCOMPAT_FEATURE_BIT            ((1ULL << 11) | (1ULL << 12) | (1ULL << 25) | (1ULL << 26) | (1ULL << 27) | (1ULL << 28))


#endif
