/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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

#include "owncp.h"

#if (_IPP32E>=_IPP32E_K1)
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning(disable: 4310) // cast truncates constant value in MSVC
#endif

#include "pcptool.h"

#include "pcpbnuimpl.h"
#include "pcpbnumisc.h"

#include "pcpngmontexpstuff.h"
#include "pcpngmontexpstuff_avx512.h"
#include "gsscramble.h"
#include "pcpmask_ct.h"

/* ams functions */
IPP_OWN_FUNPTR (void, cpAMM52, (Ipp64u* out, const Ipp64u* a, const Ipp64u* b, const Ipp64u* m, Ipp64u k0, int len, Ipp64u* res))

static void AMM52(Ipp64u* out, const Ipp64u* a, const Ipp64u* b, const Ipp64u* m, Ipp64u k0, int len, Ipp64u* res)
{
   #define NUM64  ((Ipp32s)(sizeof(__m512i)/sizeof(Ipp64u)))

   __mmask8 k1 = (__mmask8)_mm512_kmov(0x02);   /* mask of the 2-nd elment */

   __m512i zero = _mm512_setzero_si512(); /* zeros */

   int n;
   int tail = len & (NUM64 -1);
   int expLen = len;
   if(tail) expLen += (NUM64 - tail);

   /* make sure not inplace operation */
   //tbcd: temporary excluded: assert(res!=a);
   //tbcd: temporary excluded: assert(res!=b);

   /* set result to zero */
   for(n=0; n<expLen; n+=NUM64)
      _mm512_storeu_si512(res+n, zero);

   /*
   // do Almost Montgomery Multiplication
   */
   for(n=0; n<len; n++) {
      /* compute and broadcast y = (r[0]+a[0]*b[0])*k0 */
      Ipp64u y = ((res[0] + a[0]*b[n]) * k0) & EXP_DIGIT_MASK_AVX512;
      __m512i yn = _mm512_set1_epi64((Ipp64s)y);

      /* broadcast b[n] digit */
      __m512i bn = _mm512_set1_epi64((Ipp64s)b[n]);

      int i;
      __m512i rp, ap, mp, d;

      /* r[0] += a[0]*b + m[0]*y */
      __m512i ri = _mm512_loadu_si512(res);  /* r[0] */
      __m512i ai = _mm512_loadu_si512(a);    /* a[0] */
      __m512i mi = _mm512_loadu_si512(m);    /* m[0] */
      ri = _mm512_madd52lo_epu64(ri, ai, bn);
      ri = _mm512_madd52lo_epu64(ri, mi, yn);

      /* shift r[0] by 1 digit */
      d = _mm512_srli_epi64(ri, EXP_DIGIT_SIZE_AVX512);
      d = _mm512_shuffle_epi32(d, 0x44);
      d = _mm512_mask_add_epi64(ri, k1, ri, d);

      for(i=8; i<expLen; i+=8) {
         //rp = ri;
         ri = _mm512_loadu_si512(res+i);
         ap = ai;
         ai = _mm512_loadu_si512(a+i);
         mp = mi;
         mi = _mm512_loadu_si512(m+i);

         /* r[] += lo(a[]*b + m[]*y) */
         ri = _mm512_madd52lo_epu64(ri, ai, bn);
         ri = _mm512_madd52lo_epu64(ri, mi, yn);

         /* shift r[] by 1 digit */
         rp = _mm512_alignr_epi64(ri, d, 1);
         d = ri;

         /* r[] += hi(a[]*b + m[]*y) */
         rp = _mm512_madd52hi_epu64(rp, ap, bn);
         rp = _mm512_madd52hi_epu64(rp, mp, yn);
         _mm512_storeu_si512(res+i-NUM64, rp);
      }
      ri = _mm512_alignr_epi64(zero, d, 1);
      ri = _mm512_madd52hi_epu64(ri, ai, bn);
      ri = _mm512_madd52hi_epu64(ri, mi, yn);
      _mm512_storeu_si512(res+i-NUM64, ri);
   }

   /* normalization */
   {
      Ipp64u acc = 0;
      for(n=0; n<len; n++) {
         acc += res[n];
         out[n] = acc & EXP_DIGIT_MASK_AVX512;
         acc >>= EXP_DIGIT_SIZE_AVX512;
      }
   }
}

static void AMM52x20(Ipp64u* out, const Ipp64u* a, const Ipp64u* b, const Ipp64u* m, Ipp64u k0, int len, Ipp64u* res)
{
   __mmask8 k2 = (__mmask8)_mm512_kmov(0x0f);   /* mask of the 0-3 elments */

   /* load a */
   __m512i A0 = _mm512_loadu_si512(a);
   __m512i A1 = _mm512_loadu_si512(a+NUM64);
   __m512i A2 = _mm512_maskz_loadu_epi64(k2, a+2*NUM64);

   /* load m */
   __m512i M0 = _mm512_loadu_si512(m);
   __m512i M1 = _mm512_loadu_si512(m+NUM64);
   __m512i M2 = _mm512_maskz_loadu_epi64(k2, m+2*NUM64);

   /* R0, R1, R2 holds temporary result */
   __m512i R0 = _mm512_setzero_si512();
   __m512i R1 = _mm512_setzero_si512();
   __m512i R2 = _mm512_setzero_si512();

   __m512i ZERO = _mm512_setzero_si512(); /* zeros */
   __m512i K = _mm512_set1_epi64((Ipp64s)k0);

   IPP_UNREFERENCED_PARAMETER(len);
   IPP_UNREFERENCED_PARAMETER(res);

   __mmask8 k1 = (__mmask8)_mm512_kmov(0x01);   /* mask of the 0 elment */
   int i;
   for(i=0; i<20; i++) {
      __m512i Bi = _mm512_set1_epi64((Ipp64s)b[i]);     /* bloadcast(b[i]) */
      __m512i Yi = _mm512_setzero_si512();      /* Yi = 0 */
      __m512i tmp;

      R0 = _mm512_madd52lo_epu64(R0, A0, Bi);   /* R += A*Bi (lo) */
      R1 = _mm512_madd52lo_epu64(R1, A1, Bi);
      R2 = _mm512_madd52lo_epu64(R2, A2, Bi);

      Yi = _mm512_madd52lo_epu64(ZERO, K, R0);  /* Yi = R0*K */
      Yi = _mm512_permutexvar_epi64(ZERO, Yi);  /* broadcast Yi */

      R0 = _mm512_madd52lo_epu64(R0, M0, Yi);   /* R += M*Yi (lo) */
      R1 = _mm512_madd52lo_epu64(R1, M1, Yi);
      R2 = _mm512_madd52lo_epu64(R2, M2, Yi);

      /* shift R */
      tmp = _mm512_maskz_srli_epi64(k1, R0, EXP_DIGIT_SIZE_AVX512);
      R0 = _mm512_alignr_epi64(R1, R0, 1);
      R1 = _mm512_alignr_epi64(R2, R1, 1);
      R2 = _mm512_alignr_epi64(ZERO, R2, 1);
      R0 = _mm512_add_epi64(R0, tmp);

      R0 = _mm512_madd52hi_epu64(R0, A0, Bi);   /* R += A*Bi (hi) */
      R1 = _mm512_madd52hi_epu64(R1, A1, Bi);
      R2 = _mm512_madd52hi_epu64(R2, A2, Bi);

      R0 = _mm512_madd52hi_epu64(R0, M0, Yi);   /* R += M*Yi (hi) */
      R1 = _mm512_madd52hi_epu64(R1, M1, Yi);
      R2 = _mm512_madd52hi_epu64(R2, M2, Yi);
   }

   /* store de-normilized result */
   _mm512_storeu_si512(out, R0);
   _mm512_storeu_si512(out+NUM64, R1);
   _mm512_mask_storeu_epi64(out+2*NUM64, k2, R2);

   /* normalize result */
   {
      Ipp64u acc = 0;
      #if !defined(_MSC_VER) || defined(__INTEL_COMPILER) // unkonwn for msvc
      #pragma nounroll
      #endif
      for(i=0; i<20; i++) {
         acc += out[i];
         out[i] = acc & EXP_DIGIT_MASK_AVX512;
         acc >>= EXP_DIGIT_SIZE_AVX512;
      }
   }
}

static void AMM52x40(Ipp64u* out, const Ipp64u* a, const Ipp64u* b, const Ipp64u* m, Ipp64u k0, int len, Ipp64u* res)
{
   /* load a */
   __m512i A0 = _mm512_loadu_si512(a);
   __m512i A1 = _mm512_loadu_si512(a+NUM64);
   __m512i A2 = _mm512_loadu_si512(a+2*NUM64);
   __m512i A3 = _mm512_loadu_si512(a+3*NUM64);
   __m512i A4 = _mm512_loadu_si512(a+4*NUM64);

   /* load m */
   __m512i M0 = _mm512_loadu_si512(m);
   __m512i M1 = _mm512_loadu_si512(m+NUM64);
   __m512i M2 = _mm512_loadu_si512(m+2*NUM64);
   __m512i M3 = _mm512_loadu_si512(m+3*NUM64);
   __m512i M4 = _mm512_loadu_si512(m+4*NUM64);

   /* R0, R1, R2, R3, R4 holds temporary result */
   __m512i R0 = _mm512_setzero_si512();
   __m512i R1 = _mm512_setzero_si512();
   __m512i R2 = _mm512_setzero_si512();
   __m512i R3 = _mm512_setzero_si512();
   __m512i R4 = _mm512_setzero_si512();

   __m512i ZERO = _mm512_setzero_si512(); /* zeros */
   __m512i K = _mm512_set1_epi64((Ipp64s)k0);

   IPP_UNREFERENCED_PARAMETER(len);
   IPP_UNREFERENCED_PARAMETER(res);

   __mmask8 k1 = (__mmask8)_mm512_kmov(0x01);   /* mask of the 0 elment */
   int i;
   for(i=0; i<40; i++) {
      __m512i Bi = _mm512_set1_epi64((Ipp64s)b[i]);     /* bloadcast(b[i]) */
      __m512i Yi = _mm512_setzero_si512();      /* Yi = 0 */
      __m512i tmp;

      R0 = _mm512_madd52lo_epu64(R0, A0, Bi);   /* R += A*Bi (lo) */
      R1 = _mm512_madd52lo_epu64(R1, A1, Bi);
      R2 = _mm512_madd52lo_epu64(R2, A2, Bi);
      R3 = _mm512_madd52lo_epu64(R3, A3, Bi);
      R4 = _mm512_madd52lo_epu64(R4, A4, Bi);

      Yi = _mm512_madd52lo_epu64(ZERO, K, R0);  /* Yi = R0*K */
      Yi = _mm512_permutexvar_epi64(ZERO, Yi);  /* broadcast Yi */

      R0 = _mm512_madd52lo_epu64(R0, M0, Yi);   /* R += M*Yi (lo) */
      R1 = _mm512_madd52lo_epu64(R1, M1, Yi);
      R2 = _mm512_madd52lo_epu64(R2, M2, Yi);
      R3 = _mm512_madd52lo_epu64(R3, M3, Yi);
      R4 = _mm512_madd52lo_epu64(R4, M4, Yi);

      /* shift R */
      tmp = _mm512_maskz_srli_epi64(k1, R0, EXP_DIGIT_SIZE_AVX512);
      R0 = _mm512_alignr_epi64(R1, R0, 1);
      R1 = _mm512_alignr_epi64(R2, R1, 1);
      R2 = _mm512_alignr_epi64(R3, R2, 1);
      R3 = _mm512_alignr_epi64(R4, R3, 1);
      R4 = _mm512_alignr_epi64(ZERO, R4, 1);
      R0 = _mm512_add_epi64(R0, tmp);

      R0 = _mm512_madd52hi_epu64(R0, A0, Bi);   /* R += A*Bi (hi) */
      R1 = _mm512_madd52hi_epu64(R1, A1, Bi);
      R2 = _mm512_madd52hi_epu64(R2, A2, Bi);
      R3 = _mm512_madd52hi_epu64(R3, A3, Bi);
      R4 = _mm512_madd52hi_epu64(R4, A4, Bi);

      R0 = _mm512_madd52hi_epu64(R0, M0, Yi);   /* R += M*Yi (hi) */
      R1 = _mm512_madd52hi_epu64(R1, M1, Yi);
      R2 = _mm512_madd52hi_epu64(R2, M2, Yi);
      R3 = _mm512_madd52hi_epu64(R3, M3, Yi);
      R4 = _mm512_madd52hi_epu64(R4, M4, Yi);
   }

   /* store de-normilized result */
   _mm512_storeu_si512(out, R0);
   _mm512_storeu_si512(out+NUM64, R1);
   _mm512_storeu_si512(out+2*NUM64, R2);
   _mm512_storeu_si512(out+3*NUM64, R3);
   _mm512_storeu_si512(out+4*NUM64, R4);

   /* normalize result */
   {
      Ipp64u acc = 0;
      #if !defined(_MSC_VER) || defined(__INTEL_COMPILER) // unkonwn for msvc
      #pragma nounroll
      #endif
      for(i=0; i<40; i++) {
         acc += out[i];
         out[i] = acc & EXP_DIGIT_MASK_AVX512;
         acc >>= EXP_DIGIT_SIZE_AVX512;
      }
   }
}

static void AMM52x60(Ipp64u* out, const Ipp64u* a, const Ipp64u* b, const Ipp64u* m, Ipp64u k0, int len, Ipp64u* res)
{
   __mmask8 k2 = (__mmask8)_mm512_kmov(0x0f);   /* mask of the 0-3 elments */

   /* load a */
   __m512i A0 = _mm512_loadu_si512(a);
   __m512i A1 = _mm512_loadu_si512(a+NUM64);
   __m512i A2 = _mm512_loadu_si512(a+2*NUM64);
   __m512i A3 = _mm512_loadu_si512(a+3*NUM64);
   __m512i A4 = _mm512_loadu_si512(a+4*NUM64);
   __m512i A5 = _mm512_loadu_si512(a+5*NUM64);
   __m512i A6 = _mm512_loadu_si512(a+6*NUM64);
   __m512i A7 = _mm512_maskz_loadu_epi64(k2, a+7*NUM64);

   /* load m */
   __m512i M0 = _mm512_loadu_si512(m);
   __m512i M1 = _mm512_loadu_si512(m+NUM64);
   __m512i M2 = _mm512_loadu_si512(m+2*NUM64);
   __m512i M3 = _mm512_loadu_si512(m+3*NUM64);
   __m512i M4 = _mm512_loadu_si512(m+4*NUM64);
   __m512i M5 = _mm512_loadu_si512(m+5*NUM64);
   __m512i M6 = _mm512_loadu_si512(m+6*NUM64);
   __m512i M7 = _mm512_maskz_loadu_epi64(k2, m+7*NUM64);

   /* R0, R1, R2, R3, R4, R5, R6, R7 holds temporary result */
   __m512i R0 = _mm512_setzero_si512();
   __m512i R1 = _mm512_setzero_si512();
   __m512i R2 = _mm512_setzero_si512();
   __m512i R3 = _mm512_setzero_si512();
   __m512i R4 = _mm512_setzero_si512();
   __m512i R5 = _mm512_setzero_si512();
   __m512i R6 = _mm512_setzero_si512();
   __m512i R7 = _mm512_setzero_si512();

   __m512i ZERO = _mm512_setzero_si512(); /* zeros */
   __m512i K = _mm512_set1_epi64((Ipp64s)k0);

   IPP_UNREFERENCED_PARAMETER(len);
   IPP_UNREFERENCED_PARAMETER(res);

   __mmask8 k1 = (__mmask8)_mm512_kmov(0x01);   /* mask of the 0 elment */
   int i;
   for(i=0; i<60; i++) {
      __m512i Bi = _mm512_set1_epi64((Ipp64s)b[i]);     /* bloadcast(b[i]) */
      __m512i Yi = _mm512_setzero_si512();      /* Yi = 0 */
      __m512i tmp;

      R0 = _mm512_madd52lo_epu64(R0, A0, Bi);   /* R += A*Bi (lo) */
      R1 = _mm512_madd52lo_epu64(R1, A1, Bi);
      R2 = _mm512_madd52lo_epu64(R2, A2, Bi);
      R3 = _mm512_madd52lo_epu64(R3, A3, Bi);
      R4 = _mm512_madd52lo_epu64(R4, A4, Bi);
      R5 = _mm512_madd52lo_epu64(R5, A5, Bi);
      R6 = _mm512_madd52lo_epu64(R6, A6, Bi);
      R7 = _mm512_madd52lo_epu64(R7, A7, Bi);

      Yi = _mm512_madd52lo_epu64(ZERO, K, R0);  /* Yi = R0*K */
      Yi = _mm512_permutexvar_epi64(ZERO, Yi);  /* broadcast Yi */

      R0 = _mm512_madd52lo_epu64(R0, M0, Yi);   /* R += M*Yi (lo) */
      R1 = _mm512_madd52lo_epu64(R1, M1, Yi);
      R2 = _mm512_madd52lo_epu64(R2, M2, Yi);
      R3 = _mm512_madd52lo_epu64(R3, M3, Yi);
      R4 = _mm512_madd52lo_epu64(R4, M4, Yi);
      R5 = _mm512_madd52lo_epu64(R5, M5, Yi);
      R6 = _mm512_madd52lo_epu64(R6, M6, Yi);
      R7 = _mm512_madd52lo_epu64(R7, M7, Yi);

      /* shift R */
      tmp = _mm512_maskz_srli_epi64(k1, R0, EXP_DIGIT_SIZE_AVX512);
      R0 = _mm512_alignr_epi64(R1, R0, 1);
      R1 = _mm512_alignr_epi64(R2, R1, 1);
      R2 = _mm512_alignr_epi64(R3, R2, 1);
      R3 = _mm512_alignr_epi64(R4, R3, 1);
      R4 = _mm512_alignr_epi64(R5, R4, 1);
      R5 = _mm512_alignr_epi64(R6, R5, 1);
      R6 = _mm512_alignr_epi64(R7, R6, 1);
      R7 = _mm512_alignr_epi64(ZERO, R7, 1);
      R0 = _mm512_add_epi64(R0, tmp);

      R0 = _mm512_madd52hi_epu64(R0, A0, Bi);   /* R += A*Bi (hi) */
      R1 = _mm512_madd52hi_epu64(R1, A1, Bi);
      R2 = _mm512_madd52hi_epu64(R2, A2, Bi);
      R3 = _mm512_madd52hi_epu64(R3, A3, Bi);
      R4 = _mm512_madd52hi_epu64(R4, A4, Bi);
      R5 = _mm512_madd52hi_epu64(R5, A5, Bi);
      R6 = _mm512_madd52hi_epu64(R6, A6, Bi);
      R7 = _mm512_madd52hi_epu64(R7, A7, Bi);

      R0 = _mm512_madd52hi_epu64(R0, M0, Yi);   /* R += M*Yi (hi) */
      R1 = _mm512_madd52hi_epu64(R1, M1, Yi);
      R2 = _mm512_madd52hi_epu64(R2, M2, Yi);
      R3 = _mm512_madd52hi_epu64(R3, M3, Yi);
      R4 = _mm512_madd52hi_epu64(R4, M4, Yi);
      R5 = _mm512_madd52hi_epu64(R5, M5, Yi);
      R6 = _mm512_madd52hi_epu64(R6, M6, Yi);
      R7 = _mm512_madd52hi_epu64(R7, M7, Yi);
   }

   /* store de-normilized result */
   _mm512_storeu_si512(out, R0);
   _mm512_storeu_si512(out+NUM64, R1);
   _mm512_storeu_si512(out+2*NUM64, R2);
   _mm512_storeu_si512(out+3*NUM64, R3);
   _mm512_storeu_si512(out+4*NUM64, R4);
   _mm512_storeu_si512(out+5*NUM64, R5);
   _mm512_storeu_si512(out+6*NUM64, R6);
   _mm512_mask_storeu_epi64(out+7*NUM64, k2, R7);

   /* normalize result */
   {
      Ipp64u acc = 0;
      #if !defined(_MSC_VER) || defined(__INTEL_COMPILER) // unkonwn for msvc
      #pragma nounroll
      #endif
      for(i=0; i<60; i++) {
         acc += out[i];
         out[i] = acc & EXP_DIGIT_MASK_AVX512;
         acc >>= EXP_DIGIT_SIZE_AVX512;
      }
   }
}

static void AMM52x79(Ipp64u* out, const Ipp64u* a, const Ipp64u* b, const Ipp64u* m, Ipp64u k0, int len, Ipp64u* res)
{
   __mmask8 k2 = (__mmask8)_mm512_kmov(0x7f);   /* mask of the 0-7 elments */

   /* load a */
   __m512i A0 = _mm512_loadu_si512(a);
   __m512i A1 = _mm512_loadu_si512(a+NUM64);
   __m512i A2 = _mm512_loadu_si512(a+2*NUM64);
   __m512i A3 = _mm512_loadu_si512(a+3*NUM64);
   __m512i A4 = _mm512_loadu_si512(a+4*NUM64);
   __m512i A5 = _mm512_loadu_si512(a+5*NUM64);
   __m512i A6 = _mm512_loadu_si512(a+6*NUM64);
   __m512i A7 = _mm512_loadu_si512(a+7*NUM64);
   __m512i A8 = _mm512_loadu_si512(a+8*NUM64);
   __m512i A9 = _mm512_maskz_loadu_epi64(k2, a+9*NUM64);

   /* load m */
   __m512i M0 = _mm512_loadu_si512(m);
   __m512i M1 = _mm512_loadu_si512(m+NUM64);
   __m512i M2 = _mm512_loadu_si512(m+2*NUM64);
   __m512i M3 = _mm512_loadu_si512(m+3*NUM64);
   __m512i M4 = _mm512_loadu_si512(m+4*NUM64);
   __m512i M5 = _mm512_loadu_si512(m+5*NUM64);
   __m512i M6 = _mm512_loadu_si512(m+6*NUM64);
   __m512i M7 = _mm512_loadu_si512(m+7*NUM64);
   __m512i M8 = _mm512_loadu_si512(m+8*NUM64);
   __m512i M9 = _mm512_maskz_loadu_epi64(k2, m+9*NUM64);

   /* R0, R1, R2, R3, R4, R5, R6, R7, R8, R9 holds temporary result */
   __m512i R0 = _mm512_setzero_si512();
   __m512i R1 = _mm512_setzero_si512();
   __m512i R2 = _mm512_setzero_si512();
   __m512i R3 = _mm512_setzero_si512();
   __m512i R4 = _mm512_setzero_si512();
   __m512i R5 = _mm512_setzero_si512();
   __m512i R6 = _mm512_setzero_si512();
   __m512i R7 = _mm512_setzero_si512();
   __m512i R8 = _mm512_setzero_si512();
   __m512i R9 = _mm512_setzero_si512();

   __m512i ZERO = _mm512_setzero_si512(); /* zeros */
   __m512i K = _mm512_set1_epi64((Ipp64s)k0);

   IPP_UNREFERENCED_PARAMETER(len);
   IPP_UNREFERENCED_PARAMETER(res);

   __mmask8 k1 = (__mmask8)_mm512_kmov(0x01);   /* mask of the 0 elment */
   int i;
   for(i=0; i<79; i++) {
      __m512i Bi = _mm512_set1_epi64((Ipp64s)b[i]);     /* bloadcast(b[i]) */
      __m512i Yi = _mm512_setzero_si512();      /* Yi = 0 */
      __m512i tmp;

      R0 = _mm512_madd52lo_epu64(R0, A0, Bi);   /* R += A*Bi (lo) */
      R1 = _mm512_madd52lo_epu64(R1, A1, Bi);
      R2 = _mm512_madd52lo_epu64(R2, A2, Bi);
      R3 = _mm512_madd52lo_epu64(R3, A3, Bi);
      R4 = _mm512_madd52lo_epu64(R4, A4, Bi);
      R5 = _mm512_madd52lo_epu64(R5, A5, Bi);
      R6 = _mm512_madd52lo_epu64(R6, A6, Bi);
      R7 = _mm512_madd52lo_epu64(R7, A7, Bi);
      R8 = _mm512_madd52lo_epu64(R8, A8, Bi);
      R9 = _mm512_madd52lo_epu64(R9, A9, Bi);

      Yi = _mm512_madd52lo_epu64(ZERO, K, R0);  /* Yi = R0*K */
      Yi = _mm512_permutexvar_epi64(ZERO, Yi);  /* broadcast Yi */

      R0 = _mm512_madd52lo_epu64(R0, M0, Yi);   /* R += M*Yi (lo) */
      R1 = _mm512_madd52lo_epu64(R1, M1, Yi);
      R2 = _mm512_madd52lo_epu64(R2, M2, Yi);
      R3 = _mm512_madd52lo_epu64(R3, M3, Yi);
      R4 = _mm512_madd52lo_epu64(R4, M4, Yi);
      R5 = _mm512_madd52lo_epu64(R5, M5, Yi);
      R6 = _mm512_madd52lo_epu64(R6, M6, Yi);
      R7 = _mm512_madd52lo_epu64(R7, M7, Yi);
      R8 = _mm512_madd52lo_epu64(R8, M8, Yi);
      R9 = _mm512_madd52lo_epu64(R9, M9, Yi);

      /* shift R */
      tmp = _mm512_maskz_srli_epi64(k1, R0, EXP_DIGIT_SIZE_AVX512);
      R0 = _mm512_alignr_epi64(R1, R0, 1);
      R1 = _mm512_alignr_epi64(R2, R1, 1);
      R2 = _mm512_alignr_epi64(R3, R2, 1);
      R3 = _mm512_alignr_epi64(R4, R3, 1);
      R4 = _mm512_alignr_epi64(R5, R4, 1);
      R5 = _mm512_alignr_epi64(R6, R5, 1);
      R6 = _mm512_alignr_epi64(R7, R6, 1);
      R7 = _mm512_alignr_epi64(R8, R7, 1);
      R8 = _mm512_alignr_epi64(R9, R8, 1);
      R9 = _mm512_alignr_epi64(ZERO, R9, 1);
      R0 = _mm512_add_epi64(R0, tmp);

      R0 = _mm512_madd52hi_epu64(R0, A0, Bi);   /* R += A*Bi (hi) */
      R1 = _mm512_madd52hi_epu64(R1, A1, Bi);
      R2 = _mm512_madd52hi_epu64(R2, A2, Bi);
      R3 = _mm512_madd52hi_epu64(R3, A3, Bi);
      R4 = _mm512_madd52hi_epu64(R4, A4, Bi);
      R5 = _mm512_madd52hi_epu64(R5, A5, Bi);
      R6 = _mm512_madd52hi_epu64(R6, A6, Bi);
      R7 = _mm512_madd52hi_epu64(R7, A7, Bi);
      R8 = _mm512_madd52hi_epu64(R8, A8, Bi);
      R9 = _mm512_madd52hi_epu64(R9, A9, Bi);

      R0 = _mm512_madd52hi_epu64(R0, M0, Yi);   /* R += M*Yi (hi) */
      R1 = _mm512_madd52hi_epu64(R1, M1, Yi);
      R2 = _mm512_madd52hi_epu64(R2, M2, Yi);
      R3 = _mm512_madd52hi_epu64(R3, M3, Yi);
      R4 = _mm512_madd52hi_epu64(R4, M4, Yi);
      R5 = _mm512_madd52hi_epu64(R5, M5, Yi);
      R6 = _mm512_madd52hi_epu64(R6, M6, Yi);
      R7 = _mm512_madd52hi_epu64(R7, M7, Yi);
      R8 = _mm512_madd52hi_epu64(R8, M8, Yi);
      R9 = _mm512_madd52hi_epu64(R9, M9, Yi);
   }

   /* store de-normilized result */
   _mm512_storeu_si512(out, R0);
   _mm512_storeu_si512(out+NUM64, R1);
   _mm512_storeu_si512(out+2*NUM64, R2);
   _mm512_storeu_si512(out+3*NUM64, R3);
   _mm512_storeu_si512(out+4*NUM64, R4);
   _mm512_storeu_si512(out+5*NUM64, R5);
   _mm512_storeu_si512(out+6*NUM64, R6);
   _mm512_storeu_si512(out+7*NUM64, R7);
   _mm512_storeu_si512(out+8*NUM64, R8);
   _mm512_mask_storeu_epi64(out+9*NUM64, k2, R9);

   /* normalize result */
   {
      Ipp64u acc = 0;
      #if !defined(_MSC_VER) || defined(__INTEL_COMPILER) // unkonwn for msvc
      #pragma nounroll
      #endif
      for(i=0; i<79; i++) {
         acc += out[i];
         out[i] = acc & EXP_DIGIT_MASK_AVX512;
         acc >>= EXP_DIGIT_SIZE_AVX512;
      }
   }
}

/* ======= degugging section =========================================*/
//#define _EXP_AVX512_DEBUG_
#ifdef _EXP_AVX512_DEBUG_
#include "pcpmontred.h"
void debugToConvMontDomain(BNU_CHUNK_T* pR,
                     const BNU_CHUNK_T* redInp, const BNU_CHUNK_T* redM, int almMM_bitsize,
                     const BNU_CHUNK_T* pM, const BNU_CHUNK_T* pRR, int nsM, BNU_CHUNK_T k0,
                     BNU_CHUNK_T* pBuffer)
{
   Ipp64u one[32] = {
      1,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0
   };
   Ipp64u redT[32];
   int redLen = NUMBER_OF_DIGITS(almMM_bitsize, EXP_DIGIT_SIZE_AVX512);
   AMM52(redT, redInp, one, redM, k0, redLen, pBuffer);
   dig52_regular(pR, redT, almMM_bitsize);

   //cpMontMul_BNU(pR,              should be changed
   //              redT, nsM,
   //              pRR,  nsM,
   //              pM,   nsM, k0,
   //              pBuffer, 0);
   cpMul_BNU(pBuffer, pR,nsM, pRR,nsM, 0);
   cpMontRed_BNU_opt(pR, pBuffer, pM, nsM, k0);
}
#endif
/* ===================================================================*/

IPP_OWN_DEFN (cpSize, gsMontExpBinBuffer_avx512, (int modulusBits))
{
   cpSize redNum = numofVariable_avx512(modulusBits);       /* "sizeof" variable */
   cpSize redBufferNum = numofVariableBuff_avx512(redNum,8);  /* "sizeof" variable  buffer */
   return redBufferNum *8;
}

#if defined(_USE_WINDOW_EXP_)
IPP_OWN_DEFN (cpSize, gsMontExpWinBuffer_avx512, (int modulusBits))
{
   cpSize w = gsMontExp_WinSize(modulusBits);

   cpSize redNum = numofVariable_avx512(modulusBits);       /* "sizeof" variable */
   cpSize redBufferNum = numofVariableBuff_avx512(redNum,8);  /* "sizeof" variable  buffer */

   cpSize bufferNum = CACHE_LINE_SIZE/(Ipp32s)sizeof(BNU_CHUNK_T)
                    + gsGetScrambleBufferSize(redNum, w) /* pre-computed table */
                    + redBufferNum *7;                   /* addition 7 variables */
   return bufferNum;
}
#endif /* _USE_WINDOW_EXP_ */

/*
// "fast" binary montgomery exponentiation
//
// scratch buffer structure:
//    redX[redBufferLen]
//    redT[redBufferLen]
//    redY[redBufferLen]
//    redM[redBufferLen]
//    redBuffer[redBufferLen*3]
*/
IPP_OWN_DEFN (cpSize, gsMontExpBin_BNU_avx512, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);

   int modulusBitSize = BITSIZE_BNU(dataM, nsM);
   int cnvMM_bitsize = NUMBER_OF_DIGITS(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int almMM_bitsize = cnvMM_bitsize+2;
   int redLen = NUMBER_OF_DIGITS(almMM_bitsize, EXP_DIGIT_SIZE_AVX512);
   int redBufferLen = numofVariableBuff_avx512(redLen,8);

   /* allocate buffers */
   BNU_CHUNK_T* redX = pBuffer;
   BNU_CHUNK_T* redT = redX+redBufferLen;
   BNU_CHUNK_T* redY = redT+redBufferLen;
   BNU_CHUNK_T* redM = redY+redBufferLen;
   BNU_CHUNK_T* redBuffer = redM+redBufferLen;

   cpAMM52 ammFunc;
   switch (modulusBitSize) {
      case 1024: ammFunc = AMM52x20; break;
      case 2048: ammFunc = AMM52x40; break;
      case 3072: ammFunc = AMM52x60; break;
      case 4096: ammFunc = AMM52x79; break;
      default: ammFunc = AMM52; break;
   }

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU(redBuffer, redBufferLen, dataM, nsM);
   regular_dig52(redM, redBufferLen, redBuffer, almMM_bitsize);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redBuffer, 0, redBufferLen);
   SET_BIT(redBuffer, (4*redLen*EXP_DIGIT_SIZE_AVX512- 4*cnvMM_bitsize));
   regular_dig52(redY, redBufferLen, redBuffer, almMM_bitsize);

   ZEXPAND_COPY_BNU(redBuffer, redBufferLen, dataRR, nsM);
   regular_dig52(redT, redBufferLen, redBuffer, almMM_bitsize);
   ammFunc(redT, redT, redT, redM, k0, redLen, redBuffer);
   ammFunc(redT, redT, redY, redM, k0, redLen, redBuffer);

   /* convert base to Montgomery domain */
   ZEXPAND_COPY_BNU(redY, redBufferLen/*nsX+1*/, dataX, nsX);
   regular_dig52(redX, redBufferLen, redY,  almMM_bitsize);
   ammFunc(redX, redX, redT, redM, k0, redLen, redBuffer);

   /* init result */
   COPY_BNU(redY, redX, redLen);

   FIX_BNU(dataE, nsE);
   {
      /* execute most significant part pE */
      BNU_CHUNK_T eValue = dataE[nsE-1];
      int n = cpNLZ_BNU(eValue)+1;

      eValue <<= n;
      for(; n<BNU_CHUNK_BITS; n++, eValue<<=1) {
         /* squaring/multiplication: Y = Y*Y */
         ammFunc(redY, redY, redY, redM, k0, redLen, redBuffer);

         /* and multiply Y = Y*X */
         if(eValue & ((BNU_CHUNK_T)1<<(BNU_CHUNK_BITS-1)))
            ammFunc(redY, redY, redX, redM, k0, redLen, redBuffer);
      }

      /* execute rest bits of E */
      for(--nsE; nsE>0; nsE--) {
         eValue = dataE[nsE-1];

         for(n=0; n<BNU_CHUNK_BITS; n++, eValue<<=1) {
            /* squaring: Y = Y*Y */
            ammFunc(redY, redY, redY, redM, k0, redLen, redBuffer);

            /* and multiply: Y = Y*X */
            if(eValue & ((BNU_CHUNK_T)1<<(BNU_CHUNK_BITS-1)))
               ammFunc(redY, redY, redX, redM, k0, redLen, redBuffer);
         }
      }
   }

   /* convert result back to regular domain */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   redT[0] = 1;
   ammFunc(redY, redY, redT, redM, k0, redLen, redBuffer);
   dig52_regular(dataY, redY, cnvMM_bitsize);

   return nsM;
}

#if !defined(_USE_WINDOW_EXP_)
/*
// "safe" binary montgomery exponentiation
//
// scratch buffer structure:
//    redX[redBufferLen]
//    redT[redBufferLen]
//    redY[redBufferLen]
//    redM[redBufferLen]
//    redBuffer[redBufferLen*3]
*/
IPP_OWN_DEFN (cpSize, gsMontExpBin_BNU_sscm_avx512, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   int modulusBitSize = BITSIZE_BNU(dataM, nsM);
   int cnvMM_bitsize = NUMBER_OF_DIGITS(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int almMM_bitsize = cnvMM_bitsize+2;
   int redLen = NUMBER_OF_DIGITS(almMM_bitsize, EXP_DIGIT_SIZE_AVX512);
   int redBufferLen = numofVariableBuff_avx512(redLen,8);

   /* allocate buffers */
   BNU_CHUNK_T* redX = pBuffer;
   BNU_CHUNK_T* redM = redX+redBufferLen;
   BNU_CHUNK_T* redR = redM+redBufferLen;
   BNU_CHUNK_T* redT = redR+redBufferLen;
   BNU_CHUNK_T* redY = redT+redBufferLen;
   BNU_CHUNK_T* redBuffer = redY+redBufferLen;

   cpAMM52 ammFunc;
   switch (modulusBitSize) {
      case 1024: ammFunc = AMM52x20; break;
      case 2048: ammFunc = AMM52x40; break;
      case 3072: ammFunc = AMM52x60; break;
      case 4096: ammFunc = AMM52x79; break;
      default: ammFunc = AMM52; break;
   }

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU(redBuffer, redBufferLen, dataM, nsM);
   regular_dig52(redM, redBufferLen, redBuffer, almMM_bitsize);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redBuffer, 0, redBufferLen);
   SET_BIT(redBuffer, (4*redLen*EXP_DIGIT_SIZE_AVX512- 4*cnvMM_bitsize));
   regular_dig52(redY, redBufferLen, redBuffer, almMM_bitsize);

   ZEXPAND_COPY_BNU(redBuffer, redBufferLen, dataRR, nsM);
   regular_dig52(redT, redBufferLen, redBuffer, almMM_bitsize);
   ammFunc(redT, redT, redT, redM, k0, redLen, redBuffer);
   ammFunc(redT, redT, redY, redM, k0, redLen, redBuffer);

   /* convert base to Montgomery domain */
   ZEXPAND_COPY_BNU(redY, redBufferLen/*nsX+1*/, dataX, nsX);
   regular_dig52(redX, redBufferLen, redY,  almMM_bitsize);
   ammFunc(redX, redX, redT, redM, k0, redLen, redBuffer);

   /* init result */
   ZEXPAND_BNU(redR, 0, redBufferLen);
   redR[0] = 1;
   ammFunc(redR, redR, redT, redM, k0, redLen, redBuffer);
   COPY_BNU(redY, redR, redBufferLen);

   /* execute bits of E */
   for(; nsE>0; nsE--) {
      BNU_CHUNK_T eValue = dataE[nsE-1];

      int n;
      for(n=BNU_CHUNK_BITS; n>0; n--) {
         /* T = ( msb(eValue) )? X : mont(1) */
         BNU_CHUNK_T mask = cpIsMsb_ct(eValue);
         eValue <<= 1;
         cpMaskedCopyBNU_ct(redT, mask, redX, redR, redLen);

         /* squaring: Y = Y*Y */
         ammFunc(redY, redY, redY, redM, k0, redLen, redBuffer);
         /* and multiply: Y = Y * T */
         ammFunc(redY, redY, redT, redM, k0, redLen, redBuffer);
      }
   }

   /* convert result back to regular domain */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   redT[0] = 1;
   ammFunc(redY, redY, redT, redM, k0, redLen, redBuffer);
   dig52_regular(dataY, redY, cnvMM_bitsize);

   return nsM;
}

#endif /* !_USE_WINDOW_EXP_ */


#if defined(_USE_WINDOW_EXP_)
/*
// "fast" fixed-size window montgomery exponentiation
//
// scratch buffer structure:
//    precomuted table of multipliers[(1<<w)*redLen]
//    redM[redBufferLen]
//    redY[redBufferLen]
//    redT[redBufferLen]
//    redE[redBufferLen]
//    redBuffer[redBufferLen*3]
*/
IPP_OWN_DEFN (cpSize, gsMontExpWin_BNU_avx512, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);

   int modulusBitSize = BITSIZE_BNU(dataM, nsM);
   int cnvMM_bitsize = NUMBER_OF_DIGITS(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int almMM_bitsize = cnvMM_bitsize+2;
   int redLen = NUMBER_OF_DIGITS(almMM_bitsize, EXP_DIGIT_SIZE_AVX512);
   int redBufferLen = numofVariableBuff_avx512(redLen,8);

   cpSize window = gsMontExp_WinSize(bitsizeE);
   BNU_CHUNK_T wmask = (1<<window) -1;
   cpSize nPrecomute= 1<<window;
   int n;

   BNU_CHUNK_T* redE = pBuffer;
   BNU_CHUNK_T* redM = redE+redBufferLen;
   BNU_CHUNK_T* redY = redM+redBufferLen;
   BNU_CHUNK_T* redT = redY+redBufferLen;
   BNU_CHUNK_T* redBuffer = redT+redBufferLen;
   BNU_CHUNK_T* redTable = redBuffer+redBufferLen*3;

   cpAMM52 ammFunc;
   switch (modulusBitSize) {
      case 1024: ammFunc = AMM52x20; break;
      case 2048: ammFunc = AMM52x40; break;
      case 3072: ammFunc = AMM52x60; break;
      case 4096: ammFunc = AMM52x79; break;
      default: ammFunc = AMM52; break;
   }

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU(redBuffer, redBufferLen, dataM, nsM);
   regular_dig52(redM, redBufferLen, redBuffer, almMM_bitsize);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redBuffer, 0, redBufferLen);
   SET_BIT(redBuffer, (4*redLen*EXP_DIGIT_SIZE_AVX512- 4*cnvMM_bitsize));
   regular_dig52(redY, redBufferLen, redBuffer, almMM_bitsize);

   ZEXPAND_COPY_BNU(redBuffer, redBufferLen, dataRR, nsM);
   regular_dig52(redT, redBufferLen, redBuffer, almMM_bitsize);
   ammFunc(redT, redT, redT, redM, k0, redLen, redBuffer);
   ammFunc(redT, redT, redY, redM, k0, redLen, redBuffer);

   /*
      pre-compute T[i] = X^i, i=0,.., 2^w-1
   */
   ZEXPAND_BNU(redY, 0, redBufferLen);
   redY[0] = 1;
   ammFunc(redY, redY, redT, redM, k0, redLen, redBuffer);
   COPY_BNU(redTable+0, redY, redLen);

   ZEXPAND_COPY_BNU(redBuffer, redBufferLen, dataX, nsX);
   regular_dig52(redY, redBufferLen, redBuffer, almMM_bitsize);
   ammFunc(redY, redY, redT, redM, k0, redLen, redBuffer);
   COPY_BNU(redTable+redLen, redY, redLen);

   ammFunc(redT, redY, redY, redM, k0, redLen, redBuffer);
   COPY_BNU(redTable+redLen*2, redT, redLen);

   for(n=3; n<nPrecomute; n++) {
      ammFunc(redT, redT, redY, redM, k0, redLen, redBuffer);
      COPY_BNU(redTable+redLen*n, redT, redLen);
   }

   /* expand exponent */
   ZEXPAND_COPY_BNU(redE, nsE+1, dataE, nsE);
   bitsizeE = ((bitsizeE+window-1)/window) *window;

   /* exponentiation */
   {
      /* position of the 1-st (left) window */
      int eBit = bitsizeE - window;

      /* Note:  Static analysis can generate error/warning on the expression below.
      
      The value of "bitSizeE" is limited, (modulusBitSize > bitSizeE > 0),
      it is checked in initialization phase by (ippsRSA_GetSizePublickey() and ippsRSA_InitPublicKey).
      Buffer "redE" assigned for copy of dataE, is 1 (64-bit) chunk longer than size of RSA modulus,
      therefore the access "*((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)))" is always inside the boundary.
      */
      /* extract 1-st window value */
      Ipp32u eChunk = *((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)));
      int shift = eBit & 0xF;
      cpSize windowVal = (cpSize)((eChunk>>shift) &wmask);

      /* initialize result */
      ZEXPAND_COPY_BNU(redY, redBufferLen, redTable+windowVal*redLen, redLen);

      for(eBit-=window; eBit>=0; eBit-=window) {
         /* do squaring window-times */
         for(n=0; n<window; n++) {
            ammFunc(redY, redY, redY, redM, k0, redLen, redBuffer);
         }

         /* extract next window value */
         eChunk = *((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)));
         shift = eBit & 0xF;
         windowVal = (cpSize)((eChunk>>shift) &wmask);

         /* extract precomputed value and muptiply */
         if(windowVal) {
            ammFunc(redY, redY, redTable+windowVal*redLen, redM, k0, redLen, redBuffer);
         }
      }
   }

   /* convert result back */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   redT[0] = 1;
   ammFunc(redY, redY, redT, redM, k0, redLen, redBuffer);
   dig52_regular(dataY, redY, cnvMM_bitsize);

   return nsM;
}

/*
// "safe" fixed-size window montgomery exponentiation
//
// scratch buffer structure:
//    precomuted table of multipliers[(1<<w)*redLen]
//    redM[redBufferLen]
//    redY[redBufferLen]
//    redT[redBufferLen]
//    redBuffer[redBufferLen*3]
//    redE[redBufferLen]
*/
IPP_OWN_DEFN (cpSize, gsMontExpWin_BNU_sscm_avx512, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);

   int modulusBitSize = MOD_BITSIZE(pMont);
   int cnvMM_bitsize = NUMBER_OF_DIGITS(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int almMM_bitsize = cnvMM_bitsize+2;
   int redLen = NUMBER_OF_DIGITS(almMM_bitsize, EXP_DIGIT_SIZE_AVX512);
   int redBufferLen = numofVariableBuff_avx512(redLen,8);

   cpSize window = gsMontExp_WinSize(bitsizeE);
   cpSize nPrecomute= 1<<window;
   BNU_CHUNK_T wmask = (BNU_CHUNK_T)(nPrecomute -1);
   int n;

   #ifdef _EXP_AVX512_DEBUG_
   BNU_CHUNK_T dbgValue[32];
   #endif

   BNU_CHUNK_T* redTable = (BNU_CHUNK_T*)(IPP_ALIGNED_PTR(pBuffer, CACHE_LINE_SIZE));
   BNU_CHUNK_T* redM = redTable + gsGetScrambleBufferSize(redLen, window);
   BNU_CHUNK_T* redY = redM + redBufferLen;
   BNU_CHUNK_T* redT = redY + redBufferLen;
   BNU_CHUNK_T* redBuffer = redT + redBufferLen;
   BNU_CHUNK_T* redE = redBuffer + redBufferLen*3;

   cpAMM52 ammFunc;
   switch (modulusBitSize) {
      case 1024: ammFunc = AMM52x20; break;
      case 2048: ammFunc = AMM52x40; break;
      case 3072: ammFunc = AMM52x60; break;
      case 4096: ammFunc = AMM52x79; break;
      default: ammFunc = AMM52; break;
   }

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU(redBuffer, redBufferLen, dataM, nsM);
   regular_dig52(redM, redBufferLen, redBuffer, almMM_bitsize);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redBuffer, 0, redBufferLen);
   SET_BIT(redBuffer, (4*redLen*EXP_DIGIT_SIZE_AVX512- 4*cnvMM_bitsize));
   regular_dig52(redY, redBufferLen, redBuffer, almMM_bitsize);

   ZEXPAND_COPY_BNU(redBuffer, redBufferLen, dataRR, nsM);
   regular_dig52(redT, redBufferLen, redBuffer, almMM_bitsize);
   ammFunc(redT, redT, redT, redM, k0, redLen, redBuffer);
   ammFunc(redT, redT, redY, redM, k0, redLen, redBuffer);

   /*
      pre-compute T[i] = X^i, i=0,.., 2^w-1
   */
   ZEXPAND_BNU(redY, 0, redBufferLen);
   redY[0] = 1;
   ammFunc(redY, redY, redT, redM, k0, redLen, redBuffer);
   gsScramblePut(redTable, 0, redY, redLen, window);
   #ifdef _EXP_AVX512_DEBUG_
   debugToConvMontDomain(dbgValue, redY, redM, almMM_bitsize, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   ZEXPAND_COPY_BNU(redBuffer, redBufferLen, dataX, nsX);
   regular_dig52(redY, redBufferLen, redBuffer, almMM_bitsize);
   ammFunc(redY, redY, redT, redM, k0, redLen, redBuffer);
   gsScramblePut(redTable, 1, redY, redLen, window);
   #ifdef _EXP_AVX512_DEBUG_
   debugToConvMontDomain(dbgValue, redY, redM, almMM_bitsize, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   ammFunc(redT, redY, redY, redM, k0, redLen, redBuffer);
   gsScramblePut(redTable, 2, redT, redLen, window);
   #ifdef _EXP_AVX512_DEBUG_
   debugToConvMontDomain(dbgValue, redT, redM, almMM_bitsize, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   for(n=3; n<nPrecomute; n++) {
      ammFunc(redT, redT, redY, redM, k0, redLen, redBuffer);
      gsScramblePut(redTable, n, redT, redLen, window);
      #ifdef _EXP_AVX512_DEBUG_
      debugToConvMontDomain(dbgValue, redT, redM, almMM_bitsize, dataM, dataRR, nsM, k0, redBuffer);
      #endif
   }

   /* expand exponent */
   ZEXPAND_COPY_BNU(redE, nsM+1, dataE, nsE);
   bitsizeE = ((bitsizeE+window-1)/window) *window;

   /* exponentiation */
   {
      /* position of the 1-st (left) window */
      int eBit = bitsizeE - window;

      /* extract 1-st window value */
      Ipp32u eChunk = *((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)));
      int shift = eBit & 0xF;
      cpSize windowVal = (cpSize)((eChunk>>shift) &wmask);

      /* initialize result */
      gsScrambleGet_sscm(redY, redLen, redTable, windowVal, window);
      #ifdef _EXP_AVX512_DEBUG_
      debugToConvMontDomain(dbgValue, redY, redM, almMM_bitsize, dataM, dataRR, nsM, k0, redBuffer);
      #endif

      for(eBit-=window; eBit>=0; eBit-=window) {
         /* do squaring window-times */
         for(n=0; n<window; n++) {
            ammFunc(redY, redY, redY, redM, k0, redLen, redBuffer);
         }

         /* extract next window value */
         eChunk = *((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)));
         shift = eBit & 0xF;
         windowVal = (cpSize)((eChunk>>shift) &wmask);

         /* exptact precomputed value and muptiply */
         gsScrambleGet_sscm(redT, redLen, redTable, windowVal, window);
         /* muptiply */
         ammFunc(redY, redY, redT, redM, k0, redLen, redBuffer);
      }
   }

   /* convert result back */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   redT[0] = 1;
   ammFunc(redY, redY, redT, redM, k0, redLen, redBuffer);
   dig52_regular(dataY, redY, cnvMM_bitsize);

   return nsM;
}
#endif /* _USE_WINDOW_EXP_ */

#endif /* _IPP32E_K1 */
