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

#if (_IPP>=_IPP_W7)

#include "pcpbnuimpl.h"
#include "pcpbnumisc.h"

#include "pcpngmontexpstuff.h"
#include "pcpngmontexpstuff_sse2.h"
#include "gsscramble.h"
#include "pcpmask_ct.h"


//tbcd: temporary excluded: #include <assert.h>
/*
   converts regular (base = 2^32) representation (norm, normLen)
   into "redundant" (base = 2^EXP_DIGIT_SIZE_SSE2) represenrartion (red, redLen)

   return 1

   note:
   1) repLen >= (bitsize +DIGIT_SIZE-1)/DIGIT_SIZE for complete conversion
   2) regular representation should expanded by at least one zero value,
      pre-requisite: pRegular[regLen] == 0 to make conversion correct
   3) caller must provide suitable lengths of regular and redundant respresentations
      so, conversion does correct
*/
static int regular_dig27(Ipp64u* pRep27, int repLen, const Ipp32u* pRegular, int regLen)
{
   /* expected number of digit in redundant representation */
   int n = cpDigitNum_sse2(regLen*BITSIZE(Ipp32u), EXP_DIGIT_SIZE_SSE2);

   //tbcd: temporary excluded: assert(pRegular[regLen]==0);  /* test pre-requisite */

   {
      int redBit;    /* output representatin bit */
      int i;
      for(i=0, redBit=0; i<n; i++, redBit+=EXP_DIGIT_SIZE_SSE2) {
         int idx   = redBit /NORM_DIGSIZE_SSE2;   /* input digit number */
         int shift = redBit %NORM_DIGSIZE_SSE2;   /* output digit position inside input one */
         Ipp64u x = ((Ipp64u*)(pRegular+idx))[0];
         x >>= shift;
         pRep27[i] = x & EXP_DIGIT_MASK_SSE2;
      }

      /* expands by zeros if necessary */
      for(; i<repLen; i++) {
         pRep27[i] = 0;
      }

      return 1;
   }
}

/*
   converts "redundant" (base = 2^EXP_DIGIT_SIZE_SSE2) representation (pRep27, repLen)
   into regular (base = 2^32) representation (pRegular, regLen)

   note:
   caller must provide suitable lengths of regular and redundant respresentations
   so, conversion does correct
*/
static int dig27_regular(Ipp32u* pRegular, int regLen, const Ipp64u* pRep27, int repLen)
{
   int shift = EXP_DIGIT_SIZE_SSE2;
   Ipp64u x = pRep27[0];

   int idx, i;
   for(i=1, idx=0; i<repLen && idx<regLen; i++) {
      x += pRep27[i] <<shift;
      shift += EXP_DIGIT_SIZE_SSE2;
      if(shift >= NORM_DIGSIZE_SSE2) {
         pRegular[idx++] = (Ipp32u)(x & NORM_MASK_SSE2);
         x >>= NORM_DIGSIZE_SSE2;
         shift -= NORM_DIGSIZE_SSE2;
      }
   }

   if(idx<regLen)
      pRegular[idx++] = (Ipp32u)x;

   return idx;
}

/*
   normalize "redundant" representation (pUnorm, len) into (pNorm, len)
   and returns extansion
*/
static Ipp64u cpDigit27_normalize(Ipp64u* pNorm, const Ipp64u* pUnorm, int len)
{
   int n;
   Ipp64u tmp = 0;
   for(n=0; n<len; n++) {
      tmp += pUnorm[n];
      pNorm[n] = tmp & EXP_DIGIT_MASK_SSE2;
      tmp >>= EXP_DIGIT_SIZE_SSE2;
   }
   return tmp;
}

/*
   Montgomery multiplication of the redundant operands
*/
static void cpMontMul_sse2(Ipp64u* pR, const Ipp64u* pA, const Ipp64u* pB, const Ipp64u* pModulus, int mLen, BNU_CHUNK_T k0, Ipp64u* pBuffer)
{
   int extLen = mLen+1; /* len extension */
   int i;

   __m128i v_k0 = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)k0), 0x44);
   __m128i v_digMask = _mm_srli_epi64(_mm_set1_epi32(-1), 64-EXP_DIGIT_SIZE_SSE2);
   __m128i lowQword  = _mm_srli_si128(_mm_set1_epi32(-1), sizeof(Ipp64u));
   __m128i v_b0, v_y0, v_ac;

   /* clear buffer */
   v_ac = _mm_setzero_si128();
   for(i=0; i<extLen; i+=2)
      _mm_store_si128((__m128i*)(pBuffer+i), v_ac);
   _mm_storeu_si128((__m128i*)(pBuffer+mLen), v_ac);

   /* expand operands */
   _mm_storeu_si128((__m128i*)(pA+mLen), v_ac);
   _mm_storeu_si128((__m128i*)(pB+mLen), v_ac);

   /*
   // processing
   */
   for(; mLen>1; pB+=2, mLen-=2) {
      __m128i v_b1, v_y1;

      /* v_b0 = {pB[i]:pB[i]}, v_b1 = {pB[i+1]:pB[i+1]} */
      v_b1 = _mm_load_si128((__m128i*)pB);
      v_b0 = _mm_shuffle_epi32(v_b1, 0x44);
      /* v_ac = {pA[1]:pA[0]} */
      v_ac  = _mm_load_si128((__m128i*)pA);

      v_b1 = _mm_shuffle_epi32(v_b1, 0xEE);

      /* v_ac = {pA[1]:pA[0]} * {pB[i]:pB[i]} + {pR[1]:pR[0]} */
      v_ac = _mm_add_epi64(_mm_mul_epu32(v_ac, v_b0), ((__m128i*)pBuffer)[0]);
      /* v_y0 = {v_ac[0]:v_ac[0]} * {k0:k0} ) & {digMask:digMask} */
      v_y0 = _mm_mul_epu32(_mm_shuffle_epi32(v_ac, 0x44),v_k0);
      v_y0 = _mm_and_si128(v_y0, v_digMask);
      /* v_ac += v_y0 * {pM[1]:pM[0]}*/
      v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_y0, ((__m128i*)pModulus)[0]));

      /* v_ac = {0: v_ac[1]+v_ac[0]>>EXP_DIGIT_SIZE_SSE2}*/
      v_ac = _mm_add_epi64(_mm_srli_si128(v_ac, sizeof(Ipp64u)),
                           _mm_srli_epi64(_mm_and_si128(v_ac, lowQword), EXP_DIGIT_SIZE_SSE2));
      /* v_ac += {0: pA[0]} * v_b1 */
      v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_b1, _mm_cvtsi64_si128((IPP_INT64)pA[0])));

      /* v_y1 = (v_ac * v_k0) & v_digMask */
      v_y1 = _mm_and_si128(_mm_mul_epu32(v_ac, v_k0), v_digMask);

      /* v_ac += pM[0] * v_y1 */
      v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(_mm_cvtsi64_si128((IPP_INT64)pModulus[0]), v_y1));

      v_y1 = _mm_shuffle_epi32(v_y1, 0x44);

      /* pR[2] += (v_ac>>EXP_DIGIT_SIZE_SSE2) */
      v_ac = _mm_add_epi64(_mm_srli_epi64(v_ac, EXP_DIGIT_SIZE_SSE2), ((__m128i*)pBuffer)[1]);
      _mm_store_si128(((__m128i*)pBuffer)+1, v_ac);

      for(i=2; i<extLen; i+=2) {
         v_ac = _mm_load_si128((__m128i*)(pBuffer+i));

         v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_b0, ((__m128i*)(pA+i))[0]));
         v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_y0, ((__m128i*)(pModulus+i))[0]));

         v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_b1, _mm_loadu_si128((__m128i*)(pA+i-1))));
         v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_y1, _mm_loadu_si128((__m128i*)(pModulus+i-1))));

         _mm_store_si128((__m128i*)(pBuffer+i-2), v_ac);
      }
   }

   /* last b[] if is */
   if(mLen) {
      v_b0 = _mm_cvtsi64_si128((IPP_INT64)pB[0]);
      v_ac = _mm_add_epi64(_mm_cvtsi64_si128((IPP_INT64)pBuffer[0]), _mm_mul_epu32(v_b0, ((__m128i*)(pA))[0]));
      v_y0 = _mm_and_si128(_mm_mul_epu32(v_ac, v_k0), v_digMask);
      v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_y0, ((__m128i*)(pModulus))[0]));

      v_b0 = _mm_shuffle_epi32(v_b0, 0x44);
      v_y0 = _mm_shuffle_epi32(v_y0, 0x44);

      v_ac = _mm_add_epi64(_mm_srli_epi64(v_ac, EXP_DIGIT_SIZE_SSE2), _mm_loadu_si128((__m128i*)(pBuffer+1)));
      _mm_storeu_si128((__m128i*)(pBuffer+1), v_ac);

      for(i=1; i<extLen; i+=2) {
         v_ac = _mm_loadu_si128((__m128i*)(pBuffer+i));

         v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_b0, _mm_loadu_si128((__m128i*)(pA+i))));
         v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_y0, _mm_loadu_si128((__m128i*)(pModulus+i))));

         _mm_storeu_si128((__m128i*)(pBuffer+i-1), v_ac);
      }
   }

   // normalize result
   mLen = extLen-1;
   pR[mLen] = cpDigit27_normalize(pR, pBuffer, mLen);
}

/*
   Montgomery reduction of the redundant product
   (very similar to cpMontMul_sse2() )
*/
static void cpMontRed_sse2(Ipp64u* pR, Ipp64u* pProduct, const Ipp64u* pModulus, int mLen, BNU_CHUNK_T k0)
{
   int extLen = mLen+1; /* len extension */
   int i;

   __m128i v_k0 = _mm_shuffle_epi32(_mm_cvtsi32_si128((Ipp32s)k0), 0x44);
   __m128i v_digMask = _mm_srli_epi64(_mm_set1_epi32(-1), 64-EXP_DIGIT_SIZE_SSE2);
   __m128i lowQword  = _mm_srli_si128(_mm_set1_epi32(-1), sizeof(Ipp64u));
   __m128i v_y0, v_ac;

   v_ac = _mm_setzero_si128();

   /* expand product */
   _mm_storeu_si128((__m128i*)(pProduct+2*mLen), v_ac);

   /*
   // processing
   */
   for(; mLen>1; pProduct+=2, mLen-=2) {
      __m128i v_y1;

      /* v_ac = {pProd[1]:pProd[0]} */
      v_ac  = _mm_load_si128((__m128i*)pProduct);

      /* v_y0 = {v_ac[0]:v_ac[0]} * {k0:k0} ) & {digMask:digMask} */
      v_y0 = _mm_mul_epu32(_mm_shuffle_epi32(v_ac, 0x44),v_k0);
      v_y0 = _mm_and_si128(v_y0, v_digMask);

      /* v_ac += v_y0 * {pMod[1]:pMod[0]}*/
      v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_y0, ((__m128i*)pModulus)[0]));

      /* v_ac = {0: v_ac[1]+v_ac[0]>>EXP_DIGIT_SIZE_SSE2}*/
      v_ac = _mm_add_epi64(_mm_srli_si128(v_ac, sizeof(Ipp64u)),
                           _mm_srli_epi64(_mm_and_si128(v_ac, lowQword), EXP_DIGIT_SIZE_SSE2));

      /* v_y1 = (v_ac * v_k0) & v_digMask */
      v_y1 = _mm_and_si128(_mm_mul_epu32(v_ac, v_k0), v_digMask);

      /* v_ac += pM[0] * v_y1 */
      v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(_mm_cvtsi64_si128((IPP_INT64)pModulus[0]), v_y1));

      v_y1 = _mm_shuffle_epi32(v_y1, 0x44);

      /* pProd[2] += (v_ac>>EXP_DIGIT_SIZE_SSE2) */
      v_ac = _mm_add_epi64(_mm_srli_epi64(v_ac, EXP_DIGIT_SIZE_SSE2), ((__m128i*)pProduct)[1]);
      _mm_store_si128(((__m128i*)pProduct)+1, v_ac);

      for(i=2; i<extLen; i+=2) {
         v_ac = _mm_load_si128((__m128i*)(pProduct+i));

         v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_y0, ((__m128i*)(pModulus+i))[0]));
         v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_y1, _mm_loadu_si128((__m128i*)(pModulus+i-1))));

         _mm_store_si128((__m128i*)(pProduct+i), v_ac);
      }
   }

   /* last mod[] if is */
   if(mLen) {
      v_y0 = _mm_cvtsi64_si128((IPP_INT64)pProduct[0]);
      v_y0 = _mm_and_si128(_mm_mul_epu32(v_y0, v_k0), v_digMask);
      v_y0 = _mm_shuffle_epi32(v_y0, 0x44);

      v_ac = _mm_loadu_si128((__m128i*)(pProduct));
      v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_y0, ((__m128i*)(pModulus))[0]));
      /* v_ac += {v_ac[0]: v_ac[0]}>>EXP_DIGIT_SIZE_SSE2 */
      v_ac = _mm_add_epi64(v_ac, _mm_srli_epi64(_mm_shuffle_epi32(v_ac, 0x44), EXP_DIGIT_SIZE_SSE2));
      _mm_store_si128(((__m128i*)pProduct), v_ac);

      for(i=2; i<extLen; i+=2) {
         v_ac = _mm_loadu_si128((__m128i*)(pProduct+i));
         v_ac = _mm_add_epi64(v_ac, _mm_mul_epu32(v_y0, ((__m128i*)(pModulus+i))[0]));
         _mm_storeu_si128((__m128i*)(pProduct+i), v_ac);
      }

      pProduct++;
   }

   // normalize result
   mLen = extLen-1;
   pR[mLen] = cpDigit27_normalize(pR, pProduct, mLen);
}

/*
   Montgomery squaring of the redundant operand
*/
static void cpMontSqr_sse2(Ipp64u* pR, const Ipp64u* pA, const Ipp64u* pModulus, int mLen, BNU_CHUNK_T k0, Ipp64u* pScratchBuffer)
{
   //int extLen = mLen+1;
   int k;

   Ipp64u* pDbl = pScratchBuffer;
   Ipp64u* pX = (Ipp64u*)(IPP_ALIGNED_PTR(pScratchBuffer+mLen+3, 16));

   __m128i zero = _mm_setzero_si128();

   /* expand operand - it's possible, because of buffer has 2 addition entrys */
   _mm_storeu_si128((__m128i*)(pA+mLen), zero);

   /* double input operand and clean buffer */
   for(k=0; k<mLen; k+=2) {
      __m128i a = _mm_load_si128((__m128i*)(pA+k));
      _mm_store_si128((__m128i*)(pDbl+k),_mm_add_epi64(a, a));
      _mm_storeu_si128((__m128i*)(pX+2*k), zero);
      _mm_storeu_si128((__m128i*)(pX+2*k+2), zero);
   }
   _mm_storeu_si128((__m128i*)(pDbl+mLen), zero);
   _mm_storeu_si128((__m128i*)(pDbl+mLen+2), zero);

   /* squaring */
   for(k=0; k<2; k++) {
      const Ipp64u* ps = pA;
      const Ipp64u* tmpa = pA+k;
      Ipp64u* px = pX+k;

      int i, hcounter, j;
      for(i=0, hcounter=(mLen+1-k)/2; hcounter>1; i+=4,hcounter-=2, tmpa+=4,ps+=4) {
         __m128i t;
         __m128i a0 = _mm_shuffle_epi32(_mm_cvtsi64_si128 ((IPP_INT64)tmpa[0]), 0x44);   /* {a0:a0} */
         __m128i a2 = _mm_shuffle_epi32(_mm_cvtsi64_si128 ((IPP_INT64)tmpa[2]), 0x44);   /* {a2:a2} */

         __m128i s0 = _mm_load_si128((__m128i*)ps);
         __m128i s2 = _mm_load_si128((__m128i*)(ps+2));

         __m128i d0 = _mm_load_si128((__m128i*)(pDbl+i+2));
         __m128i d2 = _mm_load_si128((__m128i*)(pDbl+i+4));

         /* px[2*i]   += a0*s00; px[2*i+1] += a0*s01; */
         _mm_storeu_si128((__m128i*)(px+2*i), _mm_add_epi64(_mm_mul_epu32(a0,s0), _mm_loadu_si128((__m128i*)(px+2*i))));

         /* px[2*i+2] += a0*d00; px[2*i+3] += a0*d01; */
         _mm_storeu_si128((__m128i*)(px+2*i+2), _mm_add_epi64(_mm_mul_epu32(a0,d0), _mm_loadu_si128((__m128i*)(px+2*i+2))));

         /* px[2*i+4] += a0*d20 + a2*s20; px[2*i+5] += a0*d21 + a2*s21; */
         t = _mm_add_epi64(_mm_mul_epu32(a0,d2), _mm_mul_epu32(a2,s2));
         _mm_storeu_si128((__m128i*)(px+2*i+4), _mm_add_epi64(t, _mm_loadu_si128((__m128i*)(px+2*i+4))));

         for(j=i+6; j<mLen; j+=2) {
            d0 = _mm_load_si128((__m128i*)(pDbl+j));
            t = _mm_add_epi64(_mm_mul_epu32(a0,d0), _mm_mul_epu32(a2,d2));
            /* px[i+j]   += a0*d00 + a2*d20; px[i+j+1] += a0*d01 + a2*d21; */
            _mm_storeu_si128((__m128i*)(px+i+j), _mm_add_epi64(t, _mm_loadu_si128((__m128i*)(px+i+j))));
            d2 = d0;
         }
         /* px[i+j]   += a2*d20; px[i+j+1] += a2*d21; */
         _mm_storeu_si128((__m128i*)(px+i+j), _mm_add_epi64(_mm_mul_epu32(a2,d2), _mm_loadu_si128((__m128i*)(px+i+j))));
      }

      if(hcounter) {
         __m128i a0 = _mm_shuffle_epi32(_mm_cvtsi64_si128 ((IPP_INT64)tmpa[0]), 0x44);   /* {a0:a0} */
         __m128i s0 = _mm_load_si128((__m128i*)ps);   /* {ps[1]:ps[0]} */
         /* px[2*i] += a0*s0; px[2*i+1] += a0*s1; */
         _mm_storeu_si128((__m128i*)(px+2*i), _mm_add_epi64(_mm_mul_epu32(a0,s0), _mm_loadu_si128((__m128i*)(px+2*i))));

         for(j=i+2; j<mLen; j++) {
            /* px[i+j] += a0 * pdbl[j]; */
            __m128i r = _mm_loadu_si128((__m128i*)(px+i+j));
            r = _mm_add_epi64(r, _mm_mul_epu32(a0, ((__m128i*)(pDbl+j))[0]));
            _mm_storeu_si128((__m128i*)(px+i+j), r);
         }
      }
   }

   cpMontRed_sse2(pR, pX, pModulus, mLen, k0);
}


/* ======= degugging section =========================================*/
//#define _EXP_SSE2_DEBUG_
#ifdef _EXP_SSE2_DEBUG_
IPP_OWN_DEFN (void, debugToConvMontDomain, (BNU_CHUNK_T* pR, const Ipp64u* redInp, const Ipp64u* redM, int redLen, const BNU_CHUNK_T* pM, const BNU_CHUNK_T* pRR, int nsM, BNU_CHUNK_T k0, Ipp64u* pBuffer))
{
   Ipp64u one[152] = {
      1,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0
   };
   __ALIGN16 Ipp64u redT[152];
   cpMontMul_sse2(redT, redInp, one, redM, redLen, k0, pBuffer);
   dig27_regular((Ipp32u*)redT, nsM*sizeof(BNU_CHUNK_T)/sizeof(Ipp32u), redT, redLen);

   cpMontMul_BNU(pR,
                 (Ipp32u*)redT, nsM,
                 pRR,  nsM,
                 pM,   nsM, k0,
                 (Ipp32u*)pBuffer, 0);
}
#endif
/* ===================================================================*/

IPP_OWN_DEFN (cpSize, gsMontExpBinBuffer_sse2, (int modulusBits))
{
   cpSize redNum = numofVariable_sse2(modulusBits);      /* "sizeof" variable */
   cpSize redBufferNum = numofVariableBuff_sse2(redNum); /* "sizeof" variable  buffer */
   redBufferNum *= sizeof(Ipp64u)/sizeof(BNU_CHUNK_T);
   return redBufferNum *8           /* 7 vaiables (maybe 6 enough?) */
        + (cpSize)(16/sizeof(BNU_CHUNK_T)); /* and 16-byte alignment */
}

#if defined(_USE_WINDOW_EXP_)
IPP_OWN_DEFN (cpSize, gsMontExpWinBuffer_sse2, (int modulusBits))
{
   cpSize w = gsMontExp_WinSize(modulusBits);

   cpSize bufferNum;
   cpSize redNum = numofVariable_sse2(modulusBits);
   cpSize redBufferNum = numofVariableBuff_sse2(redNum);
   redNum *= sizeof(Ipp64u)/sizeof(BNU_CHUNK_T);
   redBufferNum *= sizeof(Ipp64u)/sizeof(BNU_CHUNK_T);

   bufferNum = (cpSize)(CACHE_LINE_SIZE/sizeof(BNU_CHUNK_T))
             + (cpSize)gsGetScrambleBufferSize(redNum, w)  /* pre-computed table */
             + redBufferNum *7;                    /* addition 7 variables */
   return bufferNum;
}
#endif /* _USE_WINDOW_EXP_ */


/*
// "fast" binary montgomery exponentiation
//
// scratch buffer structure:
//    redM[redBufferLen]
//    redX[redBufferLen]
//    redY[redBufferLen]
//    redT[redBufferLen] overlapped redBuffer[redBufferLen*3]
*/
IPP_OWN_DEFN (cpSize, gsMontExpBin_BNU_sse2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBufferT))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);

   int modulusBitSize = BITSIZE_BNU(dataM, nsM);
   int convModulusBitSize = cpDigitNum_sse2(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int modulusLen32 = BITS2WORD32_SIZE(modulusBitSize);
   int redLen = cpDigitNum_sse2(convModulusBitSize+2, EXP_DIGIT_SIZE_SSE2);
   int redBufferLen = numofVariableBuff_sse2(redLen);

   /* allocate (16-byte aligned) buffers */
   Ipp64u* redM = (Ipp64u*)(IPP_ALIGNED_PTR(pBufferT, (Ipp32s)sizeof(Ipp64u)*2));
   Ipp64u* redX = redM+redBufferLen;
   Ipp64u* redY = redX+redBufferLen;
   Ipp64u* redT = redY+redBufferLen;
   Ipp64u* redBuffer = redT;
   #ifdef _EXP_SSE2_DEBUG_
   BNU_CHUNK_T dbgValue[152];
   #endif

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redT, nsM+1, dataM, nsM);
   regular_dig27(redM, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   SET_BIT(redT, (4*redLen*EXP_DIGIT_SIZE_SSE2 - 4*convModulusBitSize));
   regular_dig27(redX, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redT, nsM+1, dataRR, nsM);
   regular_dig27(redY, redBufferLen, (Ipp32u*)redT,  modulusLen32);
   cpMontSqr_sse2(redY, redY, redM, redLen, k0, redBuffer);
   cpMontMul_sse2(redY, redY, redX, redM, redLen, k0, redBuffer);

   /* convert base to Montgomery domain */
   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redT, redBufferLen/*nsX+1*/, dataX, nsX);
   regular_dig27(redX, redBufferLen, (Ipp32u*)redT,  nsX*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));
   cpMontMul_sse2(redX, redX, redY, redM, redLen, k0, redBuffer);
   #ifdef _EXP_SSE2_DEBUG_
   debugToConvMontDomain(dbgValue, redX, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
   #endif

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
         cpMontSqr_sse2(redY, redY, redM, redLen, k0, redBuffer);
         #ifdef _EXP_SSE2_DEBUG_
         debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
         #endif

         /* and multiply Y = Y*X */
         if(eValue & ((BNU_CHUNK_T)1<<(BNU_CHUNK_BITS-1))) {
            cpMontMul_sse2(redY, redY, redX, redM, redLen, k0, redBuffer);
            #ifdef _EXP_SSE2_DEBUG_
            debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
            #endif
         }
      }

      /* execute rest bits of E */
      for(--nsE; nsE>0; nsE--) {
         eValue = dataE[nsE-1];

         for(n=0; n<BNU_CHUNK_BITS; n++, eValue<<=1) {
            /* squaring: Y = Y*Y */
            cpMontSqr_sse2(redY, redY, redM, redLen, k0, redBuffer);
            #ifdef _EXP_SSE2_DEBUG_
            debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
            #endif

            /* and multiply: Y = Y*X */
            if(eValue & ((BNU_CHUNK_T)1<<(BNU_CHUNK_BITS-1)))
               cpMontMul_sse2(redY, redY, redX, redM, redLen, k0, redBuffer);
         }
      }
   }

   /* convert result back to regular domain */
   ZEXPAND_BNU(redX, 0, redBufferLen);
   redX[0] = 1;
   cpMontMul_sse2(redY, redY, redX, redM, redLen, k0, redBuffer);
   dig27_regular((Ipp32u*)dataY, nsM*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(ipp32u)), redY, redLen);

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
IPP_OWN_DEFN (cpSize, gsMontExpBin_BNU_sscm_sse2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBufferT))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   int modulusBitSize = BITSIZE_BNU(dataM, nsM);
   int convModulusBitSize = cpDigitNum_sse2(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int modulusLen32 = BITS2WORD32_SIZE(modulusBitSize);
   int redLen = cpDigitNum_sse2(convModulusBitSize+2, EXP_DIGIT_SIZE_SSE2);
   int redBufferLen = numofVariableBuff_sse2(redLen);

   /* allocate (16-byte aligned) buffers */
   Ipp64u* redM = (Ipp64u*)(IPP_ALIGNED_PTR(pBufferT, sizeof(Ipp64u)*2));
   Ipp64u* redR = redM+redBufferLen;
   Ipp64u* redX = redR+redBufferLen;
   Ipp64u* redY = redX+redBufferLen;
   Ipp64u* redT = redY+redBufferLen;
   Ipp64u* redBuffer = redT+redBufferLen;

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redT, nsM+1, dataM, nsM);
   regular_dig27(redM, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   SET_BIT(redT, (4*redLen*EXP_DIGIT_SIZE_SSE2 - 4*convModulusBitSize));
   regular_dig27(redX, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redT, nsM+1, dataRR, nsM);
   regular_dig27(redY, redBufferLen, (Ipp32u*)redT,  modulusLen32);
   cpMontSqr_sse2(redY, redY, redM, redLen, k0, redBuffer);
   cpMontMul_sse2(redY, redY, redX, redM, redLen, k0, redBuffer);

   /* convert base to Montgomery domain */
   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redT, redBufferLen/*nsX+1*/, dataX, nsX);
   regular_dig27(redX, redBufferLen, (Ipp32u*)redT,  nsX*sizeof(BNU_CHUNK_T)/sizeof(Ipp32u));
   cpMontMul_sse2(redX, redX, redY, redM, redLen, k0, redBuffer);

   /* init result */
   ZEXPAND_BNU(redR, 0, redBufferLen);
   redR[0] = 1;
   cpMontMul_sse2(redR, redR, redY, redM, redLen, k0, redBuffer);
   COPY_BNU(redY, redR, redBufferLen);

   /* execute bits of E */
   for(; nsE>0; nsE--) {
         BNU_CHUNK_T eValue = dataE[nsE-1];


      int n;
      for(n=BNU_CHUNK_BITS; n>0; n--) {
         /* T = ( msb(eValue) )? X : mont(1) */
         BNU_CHUNK_T mask = cpIsMsb_ct(eValue);
         eValue <<= 1;
         cpMaskedCopyBNU_ct((BNU_CHUNK_T*)redT, mask, (BNU_CHUNK_T*)redX, (BNU_CHUNK_T*)redR, redLen*sizeof(Ipp64u)/sizeof(BNU_CHUNK_T));

         /* squaring: Y = Y^2 */
         cpMontSqr_sse2(redY, redY, redM, redLen, k0, redBuffer);
         /* and multiply: Y = Y * T */
         cpMontMul_sse2(redY, redY, redT, redM, redLen, k0, redBuffer);
      }
   }

   /* convert result back to regular domain */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   redT[0] = 1;
   cpMontMul_sse2(redY, redY, redT, redM, redLen, k0, redBuffer);
   dig27_regular((Ipp32u*)dataY, nsM*sizeof(BNU_CHUNK_T)/sizeof(ipp32u), redY, redLen);

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
//    redBuffer[redBufferLen*3]
//    redE[redBufferLen]
*/
IPP_OWN_DEFN (cpSize, gsMontExpWin_BNU_sse2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);

   int modulusBitSize = BITSIZE_BNU(dataM, nsM);
   int convModulusBitSize = cpDigitNum_sse2(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int modulusLen32 = BITS2WORD32_SIZE(modulusBitSize);
   int redLen = cpDigitNum_sse2(convModulusBitSize+2, EXP_DIGIT_SIZE_SSE2);
   int redBufferLen = numofVariableBuff_sse2(redLen);

   cpSize window = gsMontExp_WinSize(bitsizeE);
   cpSize nPrecomute= 1<<window;
   BNU_CHUNK_T wmask = (BNU_CHUNK_T)(nPrecomute - 1);
   int n;

   Ipp64u* redTable = (Ipp64u*)(IPP_ALIGNED_PTR(pBuffer, (Ipp32s)sizeof(Ipp64u)*2));
   Ipp64u* redM = redTable + gsGetScrambleBufferSize(redLen, window);
   Ipp64u* redY = redM + redBufferLen;
   Ipp64u* redT = redY + redBufferLen;
   Ipp64u* redBuffer = redT + redBufferLen;
   Ipp64u* redE = redBuffer + redBufferLen*3;

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redE, nsM+1, dataM, nsM);
   regular_dig27(redM, redBufferLen, (Ipp32u*)redE,  modulusLen32);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   SET_BIT(redT, (4*redLen*EXP_DIGIT_SIZE_SSE2 - 4*convModulusBitSize));
   regular_dig27(redY, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redE, nsM+1, dataRR, nsM);
   regular_dig27(redT, redBufferLen, (Ipp32u*)redE,  modulusLen32);
   cpMontSqr_sse2(redT, redT, redM, redLen, k0, redBuffer);
   cpMontMul_sse2(redT, redT, redY, redM, redLen, k0, redBuffer);

   /*
      pre-compute T[i] = X^i, i=0,.., 2^w-1
   */
   ZEXPAND_BNU(redY, 0, redBufferLen);
   redY[0] = 1;
   cpMontMul_sse2(redY, redY, redT, redM, redLen, k0, redBuffer);
   COPY_BNU(redTable+0, redY, redLen);

   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redE, redBufferLen/*nsX+1*/, dataX, nsX);
   regular_dig27(redY, redBufferLen, (Ipp32u*)redE,  nsX*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));
   cpMontMul_sse2(redY, redY, redT, redM, redLen, k0, redBuffer);
   COPY_BNU(redTable+redLen, redY, redLen);

   cpMontSqr_sse2(redT, redY, redM, redLen, k0, redBuffer);
   COPY_BNU(redTable+redLen*2, redT, redLen);

   for(n=3; n<nPrecomute; n++) {
      cpMontMul_sse2(redT, redT, redY, redM, redLen, k0, redBuffer);
      COPY_BNU(redTable+redLen*n, redT, redLen);
   }

   /* expand exponent */
   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redE, nsE+1, dataE, nsE);
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
      COPY_BNU(redY, redTable+windowVal*redLen, redLen);

      for(eBit-=window; eBit>=0; eBit-=window) {
         /* do squaring window-times */
         for(n=0; n<window; n++) {
            cpMontSqr_sse2(redY, redY, redM, redLen, k0, redBuffer);
         }

         /* extract next window value */
         eChunk = *((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)));
         shift = eBit & 0xF;
         windowVal = (cpSize)((eChunk>>shift) &wmask);

         /* precomputed value muptiplication */
         if(windowVal) {
            COPY_BNU(redT, redTable+windowVal*redLen, redLen);
            cpMontMul_sse2(redY, redY, redT, redM, redLen, k0, redBuffer);
         }
      }
   }

   /* convert result back */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   redT[0] = 1;
   cpMontMul_sse2(redY, redY, redT, redM, redLen, k0, redBuffer);
   dig27_regular((Ipp32u*)dataY, nsM*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(ipp32u)), redY, redLen);

   return nsM;
}

/*
// "safe" fixed-size window montgomery exponentiation
//
// scratch buffer structure:
//    pre-computed table
//    redM[redBufferLen]
//    redY[redBufferLen]
//    redT[redBufferLen]
//    redBuffer[redBufferLen*3]
//    redE[redBufferLen]
*/
IPP_OWN_DEFN (cpSize, gsMontExpWin_BNU_sscm_sse2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);

   int modulusBitSize = MOD_BITSIZE(pMont);
   int convModulusBitSize = cpDigitNum_sse2(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int modulusLen32 = BITS2WORD32_SIZE(modulusBitSize);
   int redLen = cpDigitNum_sse2(convModulusBitSize+2, EXP_DIGIT_SIZE_SSE2);
   int redBufferLen = numofVariableBuff_sse2(redLen);

   cpSize window = gsMontExp_WinSize(bitsizeE);
   cpSize nPrecomute= 1<<window;
   BNU_CHUNK_T wmask = (BNU_CHUNK_T)(nPrecomute -1);
   int n;

   /* CACHE_LINE_SIZE aligned redTable */
   Ipp64u* redTable = (Ipp64u*)(IPP_ALIGNED_PTR(pBuffer, CACHE_LINE_SIZE));
   /* other buffers have 16-byte alignment */
   Ipp64u* redM = redTable + gsGetScrambleBufferSize(redLen, window);
   Ipp64u* redY = redM + redBufferLen;
   Ipp64u* redT = redY + redBufferLen;
   Ipp64u* redBuffer = redT + redBufferLen;
   Ipp64u* redE = redBuffer + redBufferLen*3;

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redE, nsM+1, dataM, nsM);
   regular_dig27(redM, redBufferLen, (Ipp32u*)redE,  modulusLen32);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   SET_BIT(redT, (4*redLen*EXP_DIGIT_SIZE_SSE2 - 4*convModulusBitSize));
   regular_dig27(redY, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redE, nsM+1, dataRR, nsM);
   regular_dig27(redT, redBufferLen, (Ipp32u*)redE,  modulusLen32);
   cpMontSqr_sse2(redT, redT, redM, redLen, k0, redBuffer);
   cpMontMul_sse2(redT, redT, redY, redM, redLen, k0, redBuffer);

   /*
      pre-compute T[i] = X^i, i=0,.., 2^w-1
   */
   ZEXPAND_BNU(redY, 0, redBufferLen);
   redY[0] = 1;
   cpMontMul_sse2(redY, redY, redT, redM, redLen, k0, redBuffer);
   gsScramblePut((BNU_CHUNK_T*)redTable, 0, (BNU_CHUNK_T*)redY, redLen*(Ipp32s)(sizeof(Ipp64u)/sizeof(BNU_CHUNK_T)), window);

   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redE, redBufferLen/*nsX+1*/, dataX, nsX);
   regular_dig27(redY, redBufferLen, (Ipp32u*)redE,  nsX*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));
   cpMontMul_sse2(redY, redY, redT, redM, redLen, k0, redBuffer);
   gsScramblePut((BNU_CHUNK_T*)redTable, 1, (BNU_CHUNK_T*)redY, redLen*(Ipp32s)(sizeof(Ipp64u)/sizeof(BNU_CHUNK_T)), window);

   cpMontSqr_sse2(redT, redY, redM, redLen, k0, redBuffer);
   gsScramblePut((BNU_CHUNK_T*)redTable, 2, (BNU_CHUNK_T*)redT, redLen*(Ipp32s)(sizeof(Ipp64u)/sizeof(BNU_CHUNK_T)), window);

   for(n=3; n<nPrecomute; n++) {
      cpMontMul_sse2(redT, redT, redY, redM, redLen, k0, redBuffer);
      gsScramblePut((BNU_CHUNK_T*)redTable, n, (BNU_CHUNK_T*)redT, redLen*(Ipp32s)(sizeof(Ipp64u)/sizeof(BNU_CHUNK_T)), window);
   }

   /* expand exponent */
   ZEXPAND_COPY_BNU((BNU_CHUNK_T*)redE, nsM+1, dataE, nsE);
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
      gsScrambleGet_sscm((BNU_CHUNK_T*)redY, redLen*(Ipp32s)(sizeof(Ipp64u)/sizeof(BNU_CHUNK_T)), (BNU_CHUNK_T*)redTable, windowVal, window);

      for(eBit-=window; eBit>=0; eBit-=window) {
         /* do squaring window-times */
         for(n=0; n<window; n++) {
            cpMontSqr_sse2(redY, redY, redM, redLen, k0, redBuffer);
         }

         /* extract next window value */
         eChunk = *((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)));
         shift = eBit & 0xF;
         windowVal = (cpSize)((eChunk>>shift) &wmask);
         /* exptact precomputed value and muptiply */
         gsScrambleGet_sscm((BNU_CHUNK_T*)redT, redLen*(Ipp32s)(sizeof(Ipp64u)/sizeof(BNU_CHUNK_T)), (BNU_CHUNK_T*)redTable, windowVal, window);
         cpMontMul_sse2(redY, redY, redT, redM, redLen, k0, redBuffer);
      }
   }

   /* convert result back */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   redT[0] = 1;
   cpMontMul_sse2(redY, redY, redT, redM, redLen, k0, redBuffer);
   dig27_regular((Ipp32u*)dataY, nsM*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(ipp32u)), redY, redLen);

   return nsM;
}
#endif /* _USE_WINDOW_EXP_ */

#endif /* _IPP_W7 */
