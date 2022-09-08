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

#include <immintrin.h>
#include <internal/common/ifma_defs.h>

#define ALIGNSPEC __ALIGN64

#define  DUP2_DECL(a)   a, a
#define  DUP4_DECL(a)   DUP2_DECL(a), DUP2_DECL(a)
#define  DUP8_DECL(a)   DUP4_DECL(a), DUP4_DECL(a)


//gres: typedef unsigned __int64 UINT64;
typedef int64u UINT64;


#define VUINT64 __m512i
#define VINT64  VUINT64
#define VDOUBLE __m512d
#define VMASK_L __mmask8

#define L2D(x)  _mm512_castsi512_pd(x)
#define D2L(x)  _mm512_castpd_si512(x)

// Load int constant
#define VLOAD_L(addr)  _mm512_load_epi64((__m512i const*)&(addr)[0]);
// Load DP constant
#define VLOAD_D(addr)  _mm512_load_pd((const double*)&(addr)[0]);
#define VSTORE_L(addr, x)  _mm512_store_epi64(((__m512i*)&(addr)[0]), (x));


#define VSHL_L(x, n)  _mm512_slli_epi64((x), (n));
#define VSAR_L(x, n)  _mm512_srai_epi64((x), (n));
#define VAND_L(a, b)  _mm512_and_epi64((a), (b));
#define VADD_L(a, b)  _mm512_add_epi64((a), (b));
#define VSUB_L(a, b)  _mm512_sub_epi64((a), (b));
#define VMSUB_L(mask, a, b)  _mm512_mask_sub_epi64((a), mask, (a), (b));
#define VMADD_L(mask, a, b)  _mm512_mask_add_epi64((a), mask, (a), (b));
#define VCMPU_GT_L(a, b)  _mm512_cmpgt_epu64_mask((a), (b));
#define VCMP_GE_L(a, b)  _mm512_cmpge_epi64_mask((a), (b));


// conversion unsigned 64-bit -> double
#define VCVT_L2D(x)   _mm512_cvtepu64_pd(x)
// conversion double -> signed 64-bit
#define VCVT_D2L(x)   _mm512_cvtpd_epi64(x)

#define VADD_D(a, b)      _mm512_add_pd(a, b)
#define VMUL_D(a, b)      _mm512_mul_pd(a, b)
#define VMUL_RZ_D(a, b)   _mm512_mul_round_pd(a, b, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)
#define VMUL_RU_D(a, b)   _mm512_mul_round_pd(a, b, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC)
#define VDIV_RZ_D(a, b)   _mm512_div_round_pd(a, b, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)
#define VDIV_RU_D(a, b)   _mm512_div_round_pd(a, b, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC)
#define VROUND_RZ_D(a)    _mm512_roundscale_pd(a, 3)
#define VQFMR_D(a, b, c)  _mm512_fnmadd_pd(a, b, c)
