/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <crypto_mb/cpu_features.h>
#include <internal/common/ifma_defs.h>

/* masks of bits */
#define BIT00 0x00000001
#define BIT01 0x00000002
#define BIT02 0x00000004
#define BIT03 0x00000008
#define BIT04 0x00000010
#define BIT05 0x00000020
#define BIT06 0x00000040
#define BIT07 0x00000080
#define BIT08 0x00000100
#define BIT09 0x00000200
#define BIT10 0x00000400
#define BIT11 0x00000800
#define BIT12 0x00001000
#define BIT13 0x00002000
#define BIT14 0x00004000
#define BIT15 0x00008000
#define BIT16 0x00010000
#define BIT17 0x00020000
#define BIT18 0x00040000
#define BIT19 0x00080000
#define BIT20 0x00100000
#define BIT21 0x00200000
#define BIT22 0x00400000
#define BIT23 0x00800000
#define BIT24 0x01000000
#define BIT25 0x02000000
#define BIT26 0x04000000
#define BIT27 0x08000000
#define BIT28 0x10000000
#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000

/* index inside cpu_info[4] */
#define eax_   (0)
#define ebx_   (1)
#define ecx_   (2)
#define edx_   (3)


__INLINE void _mbcp_cpuid(int32u buf[4], int32u leaf, int32u subleaf)
{
   #ifdef __GNUC__
   __asm__ ("cpuid" : "=a" (buf[0]), "=b" (buf[1]), "=c" (buf[2]), "=d" (buf[3]) : "a" (leaf), "c" (subleaf));
   #else
   __cpuidex(buf,leaf, subleaf);
   #endif
}

static int32u _mbcp_max_cpuid_main_leaf_number(void)
{
   int32u buf[4];
   _mbcp_cpuid(buf, 0, 0);
   return buf[0];
}

#if 0
static int32u _mbcp_max_cpuid_extd_leaf_number(void)
{
   int32u buf[4];
   _mbcp_cpuid(buf, 0x80000000, 0);
   return buf[0];
}
#endif

#define XSAVE_OSXSAVE   (BIT26|BIT27)
static int32u _mbcp_xsave_support(int32u bits)
{
   int32u buf[4];
   _mbcp_cpuid(buf, 1, 0);

   if(XSAVE_OSXSAVE != (buf[ecx_] & XSAVE_OSXSAVE))
      return 0;

   int32u xcr0;

   #ifdef __GNUC__
   //asm volatile("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx");
   __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx");
   #else
   xcr0 = (int32u)_xgetbv(0);
   #endif

   return (xcr0 & bits) == bits;
}

typedef struct {
   int32u  info_idx;
   int32u  info_idx_bit;
   int64u  cpu_feature;
} cpu_feature_map;

/*
// Intel(r) Architecture Instruction Set Extension and Future Features.
// Programmomg Reference. 319433-038, March 2020
// see Table 1-5, Table 1-6
*/
static const cpu_feature_map cpu_feature_detector_1_0[] = {
   {ecx_, BIT00,  mbcpCPUID_SSE3},
   {ecx_, BIT01,  mbcpCPUID_CLMUL},
   {ecx_, BIT09,  mbcpCPUID_SSSE3},
 //{ecx_, BIT12,  mbcpCPUID_FMA},          /* supported FMA extensions using YMM*/
 //{ecx_, BIT13,  mbcpCPUID_CMPXCHG16B},
   {ecx_, BIT19,  mbcpCPUID_SSE41},
   {ecx_, BIT20,  mbcpCPUID_SSE42},
   {ecx_, BIT22,  mbcpCPUID_MOVBE},
 //{ecx_, BIT23,  mbcpCPUID_POPCNT},
   {ecx_, BIT25,  mbcpCPUID_AES},
   {ecx_, BIT28,  mbcpCPUID_AVX},
   {ecx_, BIT29,  mbcpCPUID_F16C},
   {ecx_, BIT30,  mbcpCPUID_RDRAND},

   {edx_, BIT23,  mbcpCPUID_MMX},
   {edx_, BIT25,  mbcpCPUID_SSE},
   {edx_, BIT26,  mbcpCPUID_SSE2},
};

static const cpu_feature_map cpu_feature_detector_7_0[] = {
   {ebx_, BIT03, mbcpCPUID_BMI1},
   {ebx_, BIT05, mbcpCPUID_AVX2},
   {ebx_, BIT08, mbcpCPUID_BMI2},
   {ebx_, BIT14, mbcpCPUID_MPX},
   {ebx_, BIT16, mbcpCPUID_AVX512F},
   {ebx_, BIT17, mbcpCPUID_AVX512DQ},
   {ebx_, BIT18, mbcpCPUID_RDSEED},
   {ebx_, BIT19, mbcpCPUID_ADX},
   {ebx_, BIT21, mbcpCPUID_AVX512IFMA},
   {ebx_, BIT26, mbcpCPUID_AVX512PF},
   {ebx_, BIT27, mbcpCPUID_AVX512ER},
   {ebx_, BIT28, mbcpCPUID_AVX512CD},
   {ebx_, BIT29, mbcpCPUID_SHA},
   {ebx_, BIT30, mbcpCPUID_AVX512BW},
   {ebx_, BIT31, mbcpCPUID_AVX512VL},

   {ecx_, BIT01, mbcpCPUID_AVX512VBMI},
   {ecx_, BIT06, mbcpCPUID_AVX512VBMI2},
   {ecx_, BIT08, mbcpCPUID_AVX512GFNI},
   {ecx_, BIT09, mbcpCPUID_AVX512VAES},
   {ecx_, BIT10, mbcpCPUID_AVX512VCLMUL},
 //{ecx_, BIT11, mbcpCPUID_AVX512VNNI},
 //{ecx_, BIT12, mbcpCPUID_AVX512BITALG},

   {edx_, BIT02, mbcpCPUID_AVX512_4VNNIW},
   {edx_, BIT03, mbcpCPUID_AVX512_4FMADDPS},
};

#undef eax_
#undef ebx_
#undef ecx_
#undef edx_


static int64u _mbcp_cpu_feature_detector(const int32u cpuinfo[4], const cpu_feature_map* tbl, int32u tbl_len)
{
   int64u features = 0;
   int32u n;
   for(n=0; n<tbl_len; n++) {
      int32u idx = tbl[n].info_idx;
      int32u bit = tbl[n].info_idx_bit;
      int64u f = tbl[n].cpu_feature;

      features |= (bit == (cpuinfo[idx] & bit))? f : 0;
   }
   return features;
}

#define  XSAVE_SSE_SUPPORT    (BIT01)
#define  XSAVE_AVX_SUPPORT    (BIT02)
#define  XSAVE_AVX512_SUPPORT (BIT05|BIT06|BIT07)

DLL_PUBLIC
int64u mbx_get_cpu_features(void)
{
   /* detected cpu features */
   int64u features = 0;

   /* get basic highest supported CPUID leaf number */
   int32u max_cpuid_leaf = _mbcp_max_cpuid_main_leaf_number();

   if(max_cpuid_leaf) {
      int32u cpu_info[4] = {0, 0, 0, 0};

      /* cpuid info: cpuid_1_0 */
      _mbcp_cpuid(cpu_info, 1, 0);
      features = _mbcp_cpu_feature_detector(cpu_info,
                                            cpu_feature_detector_1_0,
                                            sizeof(cpu_feature_detector_1_0)/sizeof(cpu_feature_map));
      if(_mbcp_xsave_support(XSAVE_SSE_SUPPORT|XSAVE_AVX_SUPPORT))
         features |= mbcpAVX_ENABLEDBYOS;

      if(max_cpuid_leaf>=7) {
         /* cpuid info: cpuid_7_0 */
         _mbcp_cpuid(cpu_info, 7, 0);
         features |= _mbcp_cpu_feature_detector(cpu_info,
                                                cpu_feature_detector_7_0,
                                                sizeof(cpu_feature_detector_7_0)/sizeof(cpu_feature_map));
         if((features & mbcpCPUID_AVX512F) && _mbcp_xsave_support(XSAVE_AVX512_SUPPORT))
            features |= mbcpAVX512_ENABLEDBYOS;
      }
   }

   return features;
}


// based on c-flags: -mavx512dq -mavx512ifma -mavx512f -mavx512vbmi2 -mavx512cd -mavx512bw -mbmi2
#define CRYPTO_MB_REQUIRED_CPU_FEATURES ( \
                           mbcpCPUID_BMI2 \
                         | mbcpCPUID_AVX512F \
                         | mbcpCPUID_AVX512DQ \
                         | mbcpCPUID_AVX512BW \
                         | mbcpCPUID_AVX512IFMA \
                         | mbcpCPUID_AVX512VBMI2 \
                         | mbcpAVX512_ENABLEDBYOS)

DLL_PUBLIC
int mbx_is_crypto_mb_applicable(int64u cpu_features)
{
   int64u features = cpu_features;
   if(0 == features)
      features = mbx_get_cpu_features();
   return (CRYPTO_MB_REQUIRED_CPU_FEATURES == (features & CRYPTO_MB_REQUIRED_CPU_FEATURES));
}

DLL_PUBLIC
MBX_ALGO_INFO mbx_get_algo_info(enum MBX_ALGO algo)
{
    int mbx_mb_applicable = mbx_is_crypto_mb_applicable(0);
    return (0 != mbx_mb_applicable) ? MBX_WIDTH_MB8 : 0;
}
