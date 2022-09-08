/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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

/*
//
//  Purpose:
//     Cryptography Primitive.
//     Internal Definitions of AVX512 Montgomery Exp
//
*/
#include "owncp.h"

#if (_IPP32E>=_IPP32E_K1)

#include "pcpbnuimpl.h"
#include "pcpngmontexpstuff.h"

#define RSA_AVX512_MIN_BITSIZE  (1024)
#define RSA_AVX512_MAX_BITSIZE  (13*1024)

#define EXP_DIGIT_SIZE_AVX512 (52)
#define EXP_DIGIT_BASE_AVX512 (1<<EXP_DIGIT_SIZE_AVX512)
#define EXP_DIGIT_MASK_AVX512 ((Ipp64u)0xFFFFFFFFFFFFF)

/* num of digit in "digsize" representation of "bitsize" value */
#define NUMBER_OF_DIGITS(bitsize, digsize)   (((bitsize) + (digsize)-1)/(digsize))

/* number of "EXP_DIGIT_SIZE_AVX512" chunks in "bitSize" bit string matched for AMM */
__INLINE cpSize numofVariable_avx512(int modulusBits)
{
   cpSize ammBitSize = 2 + NUMBER_OF_DIGITS(modulusBits, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   cpSize redNum = NUMBER_OF_DIGITS(ammBitSize, EXP_DIGIT_SIZE_AVX512);
   return redNum;
}

/*
 * Buffer size (in qwords) that corresponds to capacity of the full register set
 * needed to hold numofVariable_avx512() qwords.
 *
 * |regCapacity| is a capacity of a single register in qwords
 */
__INLINE int numofVariableBuff_avx512(int len, int regCapacity)
{
   int tail = len % regCapacity;
   if(0==tail) tail = regCapacity;
   return len + (regCapacity-tail);
}

/*
   converts regular (base = 2^64) representation
   into "redundant" (base = 2^DIGIT_SIZE) represenrartion
*/

/* pair of 52-bit digits occupys 13 bytes (the fact is using in implementation beloow) */
__INLINE Ipp64u getDig52(const Ipp8u* pStr, int strLen)
{
   Ipp64u digit = 0;
   for(; strLen>0; strLen--) {
      digit <<= 8;
      digit += (Ipp64u)(pStr[strLen-1]);
   }
   return digit;
}

/* regular => redundant conversion */
static void regular_dig52(Ipp64u* out, int outLen /* in qwords */, const Ipp64u* in, int inBitSize)
{
   Ipp8u* inStr = (Ipp8u*)in;

   for(; inBitSize>=(2*EXP_DIGIT_SIZE_AVX512); inBitSize-=(2*EXP_DIGIT_SIZE_AVX512), out+=2) {
      out[0] = (*(Ipp64u*)inStr) & EXP_DIGIT_MASK_AVX512;
      inStr += 6;
      out[1] = ((*(Ipp64u*)inStr) >> 4) & EXP_DIGIT_MASK_AVX512;
      inStr += 7;
      outLen -= 2;
   }
   if(inBitSize>EXP_DIGIT_SIZE_AVX512) {
      Ipp64u digit = getDig52(inStr, 7);
      out[0] = digit & EXP_DIGIT_MASK_AVX512;
      inStr += 6;
      inBitSize -= EXP_DIGIT_SIZE_AVX512;
      digit = getDig52(inStr, BITS2WORD8_SIZE(inBitSize));
      out[1] = digit>>4;
      out += 2;
      outLen -= 2;
   }
   else if(inBitSize>0) {
      out[0] = getDig52(inStr, BITS2WORD8_SIZE(inBitSize));
      out++;
      outLen--;
   }
   for(; outLen>0; outLen--,out++) out[0] = 0;
}

/*
   converts "redundant" (base = 2^DIGIT_SIZE) representation
   into regular (base = 2^64)
*/
__INLINE void putDig52(Ipp8u* pStr, int strLen, Ipp64u digit)
{
   for(; strLen>0; strLen--) {
      *pStr++ = (Ipp8u)(digit&0xFF);
      digit >>= 8;
   }
}

static void dig52_regular(Ipp64u* out, const Ipp64u* in, int outBitSize)
{
   int i;
   int outLen = BITS2WORD64_SIZE(outBitSize);
   for(i=0; i<outLen; i++) out[i] = 0;

   {
      Ipp8u* outStr = (Ipp8u*)out;
      for(; outBitSize>=(2*EXP_DIGIT_SIZE_AVX512); outBitSize-=(2*EXP_DIGIT_SIZE_AVX512), in+=2) {
         (*(Ipp64u*)outStr) = in[0];
         outStr+=6;
         (*(Ipp64u*)outStr) ^= in[1] << 4;
         outStr+=7;
      }
      if(outBitSize>EXP_DIGIT_SIZE_AVX512) {
         putDig52(outStr, 7, in[0]);
         outStr+=6;
         outBitSize -= EXP_DIGIT_SIZE_AVX512;
         putDig52(outStr, BITS2WORD8_SIZE(outBitSize), (in[1]<<4 | in[0]>>48));
      }
      else if(outBitSize) {
         putDig52(outStr, BITS2WORD8_SIZE(outBitSize), in[0]);
      }
   }
}

/* exponentiation buffer size */
#define gsMontExpBinBuffer_avx512 OWNAPI(gsMontExpBinBuffer_avx512)
   IPP_OWN_DECL (cpSize, gsMontExpBinBuffer_avx512, (int modulusBits))
#define gsMontExpWinBuffer_avx512 OWNAPI(gsMontExpWinBuffer_avx512)
   IPP_OWN_DECL (cpSize, gsMontExpWinBuffer_avx512, (int modulusBits))

/* exponentiations */
#define gsMontExpBin_BNU_avx512 OWNAPI(gsMontExpBin_BNU_avx512)
   IPP_OWN_DECL (cpSize, gsMontExpBin_BNU_avx512, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
#define gsMontExpWin_BNU_avx512 OWNAPI(gsMontExpWin_BNU_avx512)
   IPP_OWN_DECL (cpSize, gsMontExpWin_BNU_avx512, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
#define gsMontExpBin_BNU_sscm_avx512 OWNAPI(gsMontExpBin_BNU_sscm_avx512)
   IPP_OWN_DECL (cpSize, gsMontExpBin_BNU_sscm_avx512, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
#define gsMontExpWin_BNU_sscm_avx512 OWNAPI(gsMontExpWin_BNU_sscm_avx512)
   IPP_OWN_DECL (cpSize, gsMontExpWin_BNU_sscm_avx512, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))

/* dual exponentiation buffer size */
#define gsMontDualExpWinBuffer_avx512 OWNAPI(gsMontDualExpWinBuffer_avx512)
   IPP_OWN_DECL (cpSize, gsMontDualExpWinBuffer_avx512, (int modulusBits))

/* dual exponentiation */
#define gsMontDualExpWin_BNU_sscm_avx512 OWNAPI(gsMontDualExpWin_BNU_sscm_avx512)
   IPP_OWN_DECL(cpSize, gsMontDualExpWin_BNU_sscm_avx512, (BNU_CHUNK_T* dataY[2], const BNU_CHUNK_T* dataX[2], cpSize nsX[2], const BNU_CHUNK_T* dataE[2], gsModEngine* pMont[2], BNU_CHUNK_T* pBuffer))

#endif /* _IPP32E_K1 */
