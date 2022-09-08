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


#include "se_cpu_feature_defs_ext.h"
#include "se_types.h"
#include "cpu_features.h"
#include "read_xcr0.h"
#include <assert.h>


static uint64_t get_bit_from_feature_id(FeatureId feature_id)
{
    assert(feature_id >= c_feature_none);
    
    if (feature_id < c_feature_end)
    {
        return 1ULL << (feature_id - 1);
    }
    return 0;
}

void get_cpu_features_ext(uint64_t *__intel_cpu_feature_indicator)
{
    unsigned int cpuid0_eax = 0, cpuid0_ebx = 0, cpuid0_ecx = 0, cpuid0_edx = 0;
    unsigned int cpuid1_eax = 0, cpuid1_ebx = 0, cpuid1_ecx = 0, cpuid1_edx = 0;
    bool cpuid1_initialized = false;
    unsigned int cpuid7_eax = 0, cpuid7_ebx = 0, cpuid7_ecx = 0, cpuid7_edx = 0;
    bool cpuid7_initialized = false;
    unsigned int ecpuid81_eax = 0, ecpuid81_ebx = 0, ecpuid81_ecx = 0, ecpuid81_edx = 0;
    bool ecpuid81_initialized = false;
    unsigned int ecpuid88_eax = 0, ecpuid88_ebx = 0, ecpuid88_ecx = 0, ecpuid88_edx = 0;
    bool ecpuid88_initialized = false;
    unsigned int ecpuid14_eax = 0, ecpuid14_ebx = 0, ecpuid14_ecx = 0, ecpuid14_edx = 0;
    bool ecpuid14_initialized = false;
    uint64_t xfeature_mask = 0;
    bool xfeature_initialized = false;
    bool is_intel = false;
    FeatureId curr_feature;

    uint64_t cpu_feature_indicator = get_bit_from_feature_id(c_feature_generic_ia32);

    sgx_cpuid(0, &cpuid0_eax, &cpuid0_ebx, &cpuid0_ecx, &cpuid0_edx);
    if (cpuid0_eax == 0)
    {
        *__intel_cpu_feature_indicator = cpu_feature_indicator;
        return;
    }

    is_intel = (cpuid0_ebx == CPU_GENU_VAL && cpuid0_edx == CPU_INEI_VAL &&
                cpuid0_ecx == CPU_NTEL_VAL);

#define BEGIN_FEATURE(value) \
    curr_feature = c_feature_##value;

#define BEGIN_SOFT_FEATURE(value) \
    curr_feature = c_soft_feature_##value;

#define BEGIN_TEST(value) \
    curr_feature = c_feature_generic_ia32;

#define CPUID_EAX1_EDX_VALUE(value) \
    if (!cpuid1_initialized) { \
        sgx_cpuid(1, &cpuid1_eax, &cpuid1_ebx, &cpuid1_ecx, &cpuid1_edx); \
        cpuid1_initialized = true; \
    } \
    if ((cpuid1_edx & (value)) == (value)) { \
        cpu_feature_indicator |= get_bit_from_feature_id(curr_feature);

#define CPUID_EAX1_ECX_VALUE(value) \
    if (!cpuid1_initialized) { \
        sgx_cpuid(1, &cpuid1_eax, &cpuid1_ebx, &cpuid1_ecx, &cpuid1_edx); \
        cpuid1_initialized = true; \
    } \
    if ((cpuid1_ecx & (value)) == (value)) { \
        cpu_feature_indicator |= get_bit_from_feature_id(curr_feature);

#define CPUID_IS_SLT_MODEL() \
    if (!cpuid1_initialized) { \
        sgx_cpuid(1, &cpuid1_eax, &cpuid1_ebx, &cpuid1_ecx, &cpuid1_edx); \
        cpuid1_initialized = true; \
    } \
    if (CPU_MODEL(cpuid1_eax) == CPU_ATOM1 || \
        CPU_MODEL(cpuid1_eax) == CPU_ATOM2 || \
        CPU_MODEL(cpuid1_eax) == CPU_ATOM3) { \
        cpu_feature_indicator |= get_bit_from_feature_id(curr_feature);

#define CPUID_EAX7_ECX0_EBX_VALUE(value) \
    if (!cpuid7_initialized) { \
        sgx_cpuidex(7, 0, &cpuid7_eax, &cpuid7_ebx, &cpuid7_ecx, &cpuid7_edx); \
        cpuid7_initialized = true; \
    } \
    if ((cpuid7_ebx & (value)) == (value)) { \
        cpu_feature_indicator |= get_bit_from_feature_id(curr_feature);

#define CPUID_EAX7_ECX0_ECX_VALUE(value) \
    if (!cpuid7_initialized) { \
        sgx_cpuidex(7, 0, &cpuid7_eax, &cpuid7_ebx, &cpuid7_ecx, &cpuid7_edx); \
        cpuid7_initialized = true; \
    } \
    if ((cpuid7_ecx & (value)) == (value)) { \
        cpu_feature_indicator |= get_bit_from_feature_id(curr_feature);
#define CPUID_EAX7_ECX0_EDX_VALUE(value) \
    if (!cpuid7_initialized) { \
        sgx_cpuidex(7, 0, &cpuid7_eax, &cpuid7_ebx, &cpuid7_ecx, &cpuid7_edx); \
        cpuid7_initialized = true; \
    } \
    if ((cpuid7_edx & (value)) == (value)) { \
        cpu_feature_indicator |= get_bit_from_feature_id(curr_feature);

#define XGETBV_MASK(value) \
    if (!xfeature_initialized) { \
        xfeature_mask = read_xcr0(); \
        xfeature_initialized = true; \
    } \
    if ((xfeature_mask & (value)) == (value)) { \
        cpu_feature_indicator |= get_bit_from_feature_id(curr_feature);

#define CPUID_EAX80000001_ECX_VALUE(value) \
    if (!ecpuid81_initialized) { \
        sgx_cpuidex(0x80000001, 0, &ecpuid81_eax, &ecpuid81_ebx, &ecpuid81_ecx, &ecpuid81_edx); \
        ecpuid81_initialized = true; \
    } \
    if ((ecpuid81_ecx & (value)) == (value)) { \
        cpu_feature_indicator |= get_bit_from_feature_id(curr_feature);

#define CPUID_EAX80000008_EBX_VALUE(value) \
    if (!ecpuid88_initialized) { \
        sgx_cpuidex(0x80000008, 0, &ecpuid88_eax, &ecpuid88_ebx, &ecpuid88_ecx, &ecpuid88_edx); \
        ecpuid88_initialized = true; \
    } \
    if ((ecpuid88_ebx & (value)) == (value)) { \
        cpu_feature_indicator |= get_bit_from_feature_id(curr_feature);

#define CPUID_EAX14_ECX0_EBX_VALUE(value) \
    if (!ecpuid14_initialized) { \
        sgx_cpuidex(0x14, 0, &ecpuid14_eax, &ecpuid14_ebx, &ecpuid14_ecx, &ecpuid14_edx); \
        ecpuid14_initialized = true; \
    } \
    if ((ecpuid14_ebx & (value)) == (value)) { \
        cpu_feature_indicator |= get_bit_from_feature_id(curr_feature);

#define ELSE_TEST                     } else {
#define END_TEST                   }
#define END_FEATURE                }

    BEGIN_FEATURE(x87)       CPUID_EAX1_EDX_VALUE(1 << 0)    END_FEATURE
    BEGIN_FEATURE(cmov)      CPUID_EAX1_EDX_VALUE(1 << 15)   END_FEATURE
    BEGIN_FEATURE(mmx)       CPUID_EAX1_EDX_VALUE(1 << 23)   END_FEATURE

    BEGIN_FEATURE(fxsave)    CPUID_EAX1_EDX_VALUE(1 << 24)
        BEGIN_FEATURE(sse)       CPUID_EAX1_EDX_VALUE(1 << 25)   END_FEATURE
        BEGIN_FEATURE(sse2)      CPUID_EAX1_EDX_VALUE(1 << 26)   END_FEATURE
        BEGIN_FEATURE(sse3)      CPUID_EAX1_ECX_VALUE(1 << 0)    END_FEATURE
        BEGIN_FEATURE(ssse3)     CPUID_EAX1_ECX_VALUE(1 << 9)    END_FEATURE
        BEGIN_FEATURE(movbe)     CPUID_EAX1_ECX_VALUE(1 << 22)   END_FEATURE
        BEGIN_FEATURE(sse4_1)    CPUID_EAX1_ECX_VALUE(1 << 19)   END_FEATURE
        BEGIN_FEATURE(sse4_2)    CPUID_EAX1_ECX_VALUE(1 << 20)   END_FEATURE
        BEGIN_FEATURE(popcnt)    CPUID_EAX1_ECX_VALUE(1 << 23)   END_FEATURE
        BEGIN_FEATURE(pclmulqdq) CPUID_EAX1_ECX_VALUE(1 << 1)    END_FEATURE
        BEGIN_FEATURE(aes)       CPUID_EAX1_ECX_VALUE(1 << 25)   END_FEATURE
        BEGIN_FEATURE(sha)       CPUID_EAX7_ECX0_EBX_VALUE(1 << 29) END_FEATURE
    END_FEATURE // fxsave

    BEGIN_FEATURE(rdrnd)     CPUID_EAX1_ECX_VALUE(1 << 30)   END_FEATURE
    BEGIN_FEATURE(bmi)       CPUID_EAX7_ECX0_EBX_VALUE((1 << 3) | (1 << 8)) END_FEATURE
    if (is_intel) {
        BEGIN_FEATURE(sgx)       CPUID_EAX7_ECX0_EBX_VALUE(1 << 2)   END_FEATURE
        BEGIN_FEATURE(hle)       CPUID_EAX7_ECX0_EBX_VALUE(1 << 4)   END_FEATURE
        BEGIN_FEATURE(rtm)       CPUID_EAX7_ECX0_EBX_VALUE(1 << 11)  END_FEATURE
    }
    BEGIN_FEATURE(adx)       CPUID_EAX7_ECX0_EBX_VALUE(1 << 19)  END_FEATURE
    BEGIN_FEATURE(rdseed)    CPUID_EAX7_ECX0_EBX_VALUE(1 << 18)  END_FEATURE
    BEGIN_FEATURE(clwb)      CPUID_EAX7_ECX0_EBX_VALUE(1 << 24)  END_FEATURE

    if (is_intel) {
        BEGIN_FEATURE(lzcnt)     CPUID_EAX80000001_ECX_VALUE(1 << 5) END_FEATURE
    }
    BEGIN_FEATURE(wbnoinvd)  CPUID_EAX80000008_EBX_VALUE(1 << 9) END_FEATURE

    BEGIN_FEATURE(gfni)      CPUID_EAX7_ECX0_ECX_VALUE(1 << 8)   END_FEATURE
    BEGIN_FEATURE(rdpid)     CPUID_EAX7_ECX0_ECX_VALUE(1 << 22)  END_FEATURE
    BEGIN_FEATURE(shstk)     CPUID_EAX7_ECX0_ECX_VALUE(1 << 7)   END_FEATURE

    BEGIN_FEATURE(ibt)       CPUID_EAX7_ECX0_EDX_VALUE(1 << 20)  END_FEATURE
    BEGIN_FEATURE(pconfig)   CPUID_EAX7_ECX0_EDX_VALUE(1 << 18)  END_FEATURE

    BEGIN_FEATURE(ptwrite)   CPUID_EAX14_ECX0_EBX_VALUE(1 << 4)  END_FEATURE

    BEGIN_TEST(xsave)        CPUID_EAX1_ECX_VALUE(1 << 27)
        BEGIN_TEST(ymm)          XGETBV_MASK((1 << 2) | (1 << 1))
            BEGIN_FEATURE(avx1)      CPUID_EAX1_ECX_VALUE(1 << 28)
                // SDM requires avx & aes to be checked before vaes bits are checked.
                // Symmetrically it requires avx & pclmulqdq to checked prior of
                // vpclmulqdq check.  We leave this semantics to be implemented as a
                // future improvements.
                BEGIN_FEATURE(vaes)       CPUID_EAX7_ECX0_ECX_VALUE(1 << 9) END_FEATURE
                BEGIN_FEATURE(vpclmulqdq) CPUID_EAX7_ECX0_ECX_VALUE(1 << 10) END_FEATURE
            END_FEATURE // avx1
            BEGIN_FEATURE(f16c)      CPUID_EAX1_ECX_VALUE(1 << 29)   END_FEATURE
            BEGIN_FEATURE(avx2)      CPUID_EAX7_ECX0_EBX_VALUE(1 << 5) END_FEATURE
            BEGIN_FEATURE(fma)       CPUID_EAX1_ECX_VALUE(1 << 12)   END_FEATURE

            BEGIN_TEST(bnd)          XGETBV_MASK(0x18)
                BEGIN_FEATURE(mpx)       CPUID_EAX7_ECX0_EBX_VALUE(1 << 14) END_FEATURE
            END_TEST // bnd

            BEGIN_TEST(zmm)          XGETBV_MASK((0x10 << 3) | (0x10 << 2) | (0x10 << 1))
                BEGIN_FEATURE(avx512f)   CPUID_EAX7_ECX0_EBX_VALUE(1 << 16) END_FEATURE
                BEGIN_FEATURE(vconflict) CPUID_EAX7_ECX0_EBX_VALUE(1 << 28) END_FEATURE
                BEGIN_FEATURE(tvx_trans) CPUID_EAX7_ECX0_EBX_VALUE(1 << 27) END_FEATURE
                BEGIN_FEATURE(tvx_prefetch) CPUID_EAX7_ECX0_EBX_VALUE(1 << 26) END_FEATURE
                BEGIN_FEATURE(avx512dq)  CPUID_EAX7_ECX0_EBX_VALUE(1 << 17) END_FEATURE
                BEGIN_FEATURE(avx512bw)  CPUID_EAX7_ECX0_EBX_VALUE(1 << 30) END_FEATURE
                BEGIN_FEATURE(avx512vl)  CPUID_EAX7_ECX0_EBX_VALUE((1ULL << 31)) END_FEATURE
                BEGIN_FEATURE(avx512ifma52) CPUID_EAX7_ECX0_EBX_VALUE(1 << 21) END_FEATURE
                BEGIN_FEATURE(avx512vbmi) CPUID_EAX7_ECX0_ECX_VALUE(1 << 1) END_FEATURE
                BEGIN_FEATURE(avx512_vpopcntdq) CPUID_EAX7_ECX0_ECX_VALUE(1 << 14) END_FEATURE
                BEGIN_FEATURE(avx512_4vnniw) CPUID_EAX7_ECX0_EDX_VALUE(1 << 2) END_FEATURE
                BEGIN_FEATURE(avx512_4fmaps) CPUID_EAX7_ECX0_EDX_VALUE(1 << 3) END_FEATURE
                BEGIN_FEATURE(avx512_bitalg) CPUID_EAX7_ECX0_ECX_VALUE(1 << 12) END_FEATURE
                BEGIN_FEATURE(avx512vbmi2) CPUID_EAX7_ECX0_ECX_VALUE(1 << 6) END_FEATURE
                BEGIN_FEATURE(avx512vnni) CPUID_EAX7_ECX0_ECX_VALUE(1 << 11) END_FEATURE

            END_TEST // zmm
        END_TEST // ymm
    END_TEST // xsave

    BEGIN_SOFT_FEATURE(proc_full_inorder) CPUID_IS_SLT_MODEL() END_FEATURE


    // Set cpu feature indicator after collection
    *__intel_cpu_feature_indicator = cpu_feature_indicator;

}
