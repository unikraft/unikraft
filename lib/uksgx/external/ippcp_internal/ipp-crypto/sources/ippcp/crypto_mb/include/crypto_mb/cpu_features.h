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

#ifndef CPU_FEATURES_H
#define CPU_FEATURES_H

#include <crypto_mb/defs.h>

/* definition of cpu features */
#define   mbcpCPUID_MMX             0x00000001LL   /* Intel® architecture with MMX(TM) technology supported                     */
#define   mbcpCPUID_SSE             0x00000002LL   /* Intel® Streaming SIMD Extensions (Intel® SSE) instruction set             */
#define   mbcpCPUID_SSE2            0x00000004LL   /* Intel® Streaming SIMD Extensions 2 (Intel® SSE2) instruction set          */
#define   mbcpCPUID_SSE3            0x00000008LL   /* Intel® Streaming SIMD Extensions 3 (Intel® SSE3) instruction set          */
#define   mbcpCPUID_SSSE3           0x00000010LL   /* Supplemental Streaming SIMD Extensions 3 (SSSE3) instruction set          */
#define   mbcpCPUID_MOVBE           0x00000020LL   /* Intel® instruction MOVBE                                                  */
#define   mbcpCPUID_SSE41           0x00000040LL   /* Intel® Streaming SIMD Extensions 4.1 (Intel® SSE4.1) instruction set      */
#define   mbcpCPUID_SSE42           0x00000080LL   /* Intel® Streaming SIMD Extensions 4.2 (Intel® SSE4.2) instruction set      */
#define   mbcpCPUID_AVX             0x00000100LL   /* Intel® Advanced Vector Extensions instruction set                         */
#define   mbcpAVX_ENABLEDBYOS       0x00000200LL   /* Intel® Advanced Vector Extensions instruction set is supported by OS      */
#define   mbcpCPUID_AES             0x00000400LL   /* AES                                                                       */
#define   mbcpCPUID_CLMUL           0x00000800LL   /* Intel® instruction PCLMULQDQ                                              */
#define   mbcpCPUID_ABR             0x00001000LL   /* Reserved                                                                  */
#define   mbcpCPUID_RDRAND          0x00002000LL   /* Intel® instruction RDRAND                                                 */
#define   mbcpCPUID_F16C            0x00004000LL   /* Intel® instruction F16C                                                   */
#define   mbcpCPUID_AVX2            0x00008000LL   /* Intel® Advanced Vector Extensions 2                                       */
#define   mbcpCPUID_ADX             0x00010000LL   /* Intel® instructions ADOX/ADCX                                             */
#define   mbcpCPUID_RDSEED          0x00020000LL   /* Intel® instruction RDSEED                                                 */
#define   mbcpCPUID_PREFETCHW       0x00040000LL   /* Intel® instruction PREFETCHW                                              */
#define   mbcpCPUID_SHA             0x00080000LL   /* Intel® Secure Hash Algorithm Extensions                                   */
#define   mbcpCPUID_AVX512F         0x00100000LL   /* Intel® Advanced Vector Extensions 512 Foundation instruction set          */
#define   mbcpCPUID_AVX512CD        0x00200000LL   /* Intel® Advanced Vector Extensions 512 CD instruction set                  */
#define   mbcpCPUID_AVX512ER        0x00400000LL   /* Intel® Advanced Vector Extensions 512 ER instruction set                  */
#define   mbcpCPUID_AVX512PF        0x00800000LL   /* Intel® Advanced Vector Extensions 512 PF instruction set                  */
#define   mbcpCPUID_AVX512BW        0x01000000LL   /* Intel® Advanced Vector Extensions 512 BW instruction set                  */
#define   mbcpCPUID_AVX512DQ        0x02000000LL   /* Intel® Advanced Vector Extensions 512 DQ instruction set                  */
#define   mbcpCPUID_AVX512VL        0x04000000LL   /* Intel® Advanced Vector Extensions 512 VL instruction set                  */
#define   mbcpCPUID_AVX512VBMI      0x08000000LL   /* Intel® Advanced Vector Extensions 512 Bit Manipulation instructions       */
#define   mbcpCPUID_MPX             0x10000000LL   /* Intel® Memory Protection Extensions                                       */
#define   mbcpCPUID_AVX512_4FMADDPS 0x20000000LL   /* Intel® Advanced Vector Extensions 512 DL floating-point single precision  */
#define   mbcpCPUID_AVX512_4VNNIW   0x40000000LL   /* Intel® Advanced Vector Extensions 512 DL enhanced word variable precision */
#define   mbcpCPUID_KNC             0x80000000LL   /* Intel® Xeon Phi(TM) Coprocessor                                           */
#define   mbcpCPUID_AVX512IFMA     0x100000000LL   /* Intel® Advanced Vector Extensions 512 IFMA (PMADD52) instruction set      */
#define   mbcpAVX512_ENABLEDBYOS   0x200000000LL   /* Intel® Advanced Vector Extensions 512 is supported by OS                  */
#define   mbcpCPUID_AVX512GFNI     0x400000000LL   /* GFNI                                                                      */
#define   mbcpCPUID_AVX512VAES     0x800000000LL   /* VAES                                                                      */
#define   mbcpCPUID_AVX512VCLMUL  0x1000000000LL   /* VCLMUL                                                                    */
#define   mbcpCPUID_AVX512VBMI2   0x2000000000LL   /* Intel® Advanced Vector Extensions 512 Bit Manipulation instructions 2     */
#define   mbcpCPUID_BMI1          0x4000000000LL   /* BMI1 */
#define   mbcpCPUID_BMI2          0x8000000000LL   /* BMI2 */

/* map cpu features */
EXTERN_C int64u mbx_get_cpu_features(void);

/* check if crypto_mb is applicable */
EXTERN_C int mbx_is_crypto_mb_applicable(int64u cpu_features);

/* supported algorithm */
enum MBX_ALGO {
    MBX_ALGO_RSA_1K,
    MBX_ALGO_RSA_2K,
    MBX_ALGO_RSA_3K,
    MBX_ALGO_RSA_4K,
    MBX_ALGO_X25519,
    MBX_ALGO_EC_NIST_P256,
    MBX_ALGO_ECDHE_NIST_P256 = MBX_ALGO_EC_NIST_P256,
    MBX_ALGO_ECDSA_NIST_P256 = MBX_ALGO_EC_NIST_P256,
    MBX_ALGO_EC_NIST_P384,
    MBX_ALGO_ECDHE_NIST_P384 = MBX_ALGO_EC_NIST_P384,
    MBX_ALGO_ECDSA_NIST_P384 = MBX_ALGO_EC_NIST_P384,
    MBX_ALGO_EC_NIST_P521,
    MBX_ALGO_ECDHE_NIST_P521 = MBX_ALGO_EC_NIST_P521,
    MBX_ALGO_ECDSA_NIST_P521 = MBX_ALGO_EC_NIST_P521
};

/* multi-buffer width implemented by library */
enum MBX_WIDTH {
    MBX_WIDTH_MB8   = 0x4,
    MBX_WIDTH_ANY   = (1 << 16) - 1
};

typedef int64u MBX_ALGO_INFO;

/* check if algorithm is supported on current platform
 * returns: multi-buffer width mask or 0 if algorithm not supported
*/
EXTERN_C MBX_ALGO_INFO mbx_get_algo_info(enum MBX_ALGO algo);

#endif /* CPU_FEATURES_H */
