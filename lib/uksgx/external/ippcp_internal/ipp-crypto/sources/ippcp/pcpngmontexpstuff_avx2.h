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

/*
//
//  Purpose:
//     Cryptography Primitive.
//     Internal Definitions of AVX2 Montgomery Exp
//
*/
#include "owncp.h"

#if (_IPP32E>=_IPP32E_L9)

#include "pcpbnuimpl.h"
#include "pcpngmontexpstuff.h"

#define RSA_AVX2_MIN_BITSIZE  (1024)
#define RSA_AVX2_MAX_BITSIZE  (13*1024)

#define NORM_DIGSIZE_AVX2 (BITSIZE(Ipp32u))
#define NORM_BASE_AVX2    ((Ipp64u)1<<NORM_DIGSIZE_AVX2)
#define NORM_MASK_AVX2    (NORM_BASE_AVX2-1)

#define EXP_DIGIT_SIZE_AVX2   (27)
#define EXP_DIGIT_BASE_AVX2   (1<<EXP_DIGIT_SIZE_AVX2)
#define EXP_DIGIT_MASK_AVX2   (EXP_DIGIT_BASE_AVX2-1)


/* number of "diSize" chunks in "bitSize" bit string */
__INLINE int cpDigitNum_avx2(int bitSize, int digSize)
{ return (bitSize + digSize-1)/digSize; }

/* number of "EXP_DIGIT_SIZE_AVX2" chunks in "bitSize" bit string matched for AMM */
__INLINE cpSize numofVariable_avx2(int modulusBits)
{
   cpSize ammBitSize = 2 + cpDigitNum_avx2(modulusBits, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   cpSize redNum = cpDigitNum_avx2(ammBitSize, EXP_DIGIT_SIZE_AVX2);
   return redNum;
}

/* buffer corresponding to numofVariable_avx2() */
__INLINE cpSize numofVariableBuff_avx2(int numV)
{
   return numV +4;
}

/* basic operations */
#define cpMontMul1024_avx2 OWNAPI(cpMontMul1024_avx2)
   IPP_OWN_DECL (void, cpMontMul1024_avx2, (Ipp64u* pR, const Ipp64u* pA, const Ipp64u* pB, const Ipp64u* pModulus, int mLen, Ipp64u m0))
#define cpMontMul4n_avx2   OWNAPI(cpMontMul4n_avx2)
   IPP_OWN_DECL (void, cpMontMul4n_avx2, (Ipp64u* pR, const Ipp64u* pA, const Ipp64u* pB, const Ipp64u* pModulus, int mLen, Ipp64u m0, Ipp64u* pScratchBuffer))
#define cpMontMul4n1_avx2  OWNAPI(cpMontMul4n1_avx2)
   IPP_OWN_DECL (void, cpMontMul4n1_avx2, (Ipp64u* pR, const Ipp64u* pA, const Ipp64u* pB, const Ipp64u* pModulus, int mLen, Ipp64u m0, Ipp64u* pScratchBuffer))
#define cpMontMul4n2_avx2  OWNAPI(cpMontMul4n2_avx2)
   IPP_OWN_DECL (void, cpMontMul4n2_avx2, (Ipp64u* pR, const Ipp64u* pA, const Ipp64u* pB, const Ipp64u* pModulus, int mLen, Ipp64u m0, Ipp64u* pScratchBuffer))
#define cpMontMul4n3_avx2  OWNAPI(cpMontMul4n3_avx2)
   IPP_OWN_DECL (void, cpMontMul4n3_avx2, (Ipp64u* pR, const Ipp64u* pA, const Ipp64u* pB, const Ipp64u* pModulus, int mLen, Ipp64u m0, Ipp64u* pScratchBuffer))

#define cpMontSqr1024_avx2 OWNAPI(cpMontSqr1024_avx2)
   IPP_OWN_DECL (void, cpMontSqr1024_avx2, (Ipp64u* pR, const Ipp64u* pA, const Ipp64u* pModulus, int mLen, Ipp64u k0, Ipp64u* pBuffer))
#define cpSqr1024_avx2  OWNAPI(cpSqr1024_avx2)
   IPP_OWN_DECL (void, cpSqr1024_avx2, (Ipp64u* pR, const Ipp64u* pA, int aLen, Ipp64u* pBuffer))
#define cpSqr_avx2  OWNAPI(cpSqr_avx2)
   IPP_OWN_DECL (void, cpSqr_avx2, (Ipp64u* pR, const Ipp64u* pA, int aLen, Ipp64u* pBuffer))

#define cpMontRed_avx2 OWNAPI(cpMontRed_avx2)
   IPP_OWN_DECL (void, cpMontRed_avx2, (Ipp64u* pR, Ipp64u* pProduct, const Ipp64u* pModulus, int mLen, Ipp64u k0))

/* exponentiation buffer size */
#define gsMontExpBinBuffer_avx2 OWNAPI(gsMontExpBinBuffer_avx2)
   IPP_OWN_DECL (cpSize, gsMontExpBinBuffer_avx2, (int modulusBits))
#define gsMontExpWinBuffer_avx2 OWNAPI(gsMontExpWinBuffer_avx2)
   IPP_OWN_DECL (cpSize, gsMontExpWinBuffer_avx2, (int modulusBits))

/* AVX2 exponentiations */
#define gsMontExpBin_BNU_avx2 OWNAPI(gsMontExpBin_BNU_avx2)
   IPP_OWN_DECL (cpSize, gsMontExpBin_BNU_avx2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
#define gsMontExpBin_BNU_sscm_avx2 OWNAPI(gsMontExpBin_BNU_sscm_avx2)
   IPP_OWN_DECL (cpSize, gsMontExpBin_BNU_sscm_avx2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
#define gsMontExpWin_BNU_avx2 OWNAPI(gsMontExpWin_BNU_avx2)
   IPP_OWN_DECL (cpSize, gsMontExpWin_BNU_avx2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
#define gsMontExpWin_BNU_sscm_avx2 OWNAPI(gsMontExpWin_BNU_sscm_avx2)
   IPP_OWN_DECL (cpSize, gsMontExpWin_BNU_sscm_avx2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))

#endif /* _IPP32E_L9 */
