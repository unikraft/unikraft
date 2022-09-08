/*******************************************************************************
* Copyright 2012-2021 Intel Corporation
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
//  Purpose:
//     Intel(R) Integrated Performance Primitives.
//     Internal Unsigned internal arithmetic
// 
// 
*/

#if !defined(_CP_BNU_ARITH_H)
#define _CP_BNU_ARITH_H

#include "pcpbnuimpl.h"
#include "pcpbnu32arith.h"

#define cpAdd_BNU OWNAPI(cpAdd_BNU)
   IPP_OWN_DECL (BNU_CHUNK_T, cpAdd_BNU, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, cpSize ns))
#define cpSub_BNU OWNAPI(cpSub_BNU)
   IPP_OWN_DECL (BNU_CHUNK_T, cpSub_BNU, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, cpSize ns))
#define cpInc_BNU OWNAPI(cpInc_BNU)
   IPP_OWN_DECL (BNU_CHUNK_T, cpInc_BNU, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, cpSize ns, BNU_CHUNK_T val))
#define cpDec_BNU OWNAPI(cpDec_BNU)
   IPP_OWN_DECL (BNU_CHUNK_T, cpDec_BNU, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, cpSize ns, BNU_CHUNK_T val))
#define cpAddMulDgt_BNU OWNAPI(cpAddMulDgt_BNU)
   IPP_OWN_DECL (BNU_CHUNK_T, cpAddMulDgt_BNU, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, cpSize ns, BNU_CHUNK_T val))
#define cpMulAdc_BNU_school OWNAPI(cpMulAdc_BNU_school)
   IPP_OWN_DECL (BNU_CHUNK_T, cpMulAdc_BNU_school, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, cpSize nsA, const BNU_CHUNK_T* pB, cpSize nsB))
#define cpMulAdx_BNU_school OWNAPI(cpMulAdx_BNU_school)
   IPP_OWN_DECL (BNU_CHUNK_T, cpMulAdx_BNU_school, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, cpSize nsA, const BNU_CHUNK_T* pB, cpSize nsB))

/*F*
//    Name: cpMul_BNU_school
//
// Purpose: Multiply 2 BigNums.
//
// Returns:
//    extension of result of multiply 2 BigNums
//
// Parameters:
//    pA    source BigNum A
//    nsA   size of A
//    pB    source BigNum B
//    nsB   size of B
//    pR    resultant BigNum
//
*F*/

__INLINE BNU_CHUNK_T cpMul_BNU_school(BNU_CHUNK_T* pR,
                                const BNU_CHUNK_T* pA, cpSize nsA,
                                const BNU_CHUNK_T* pB, cpSize nsB)
{
#if(_ADCOX_NI_ENABLING_==_FEATURE_ON_)
   return cpMulAdx_BNU_school(pR, pA,nsA, pB,nsB);
#elif(_ADCOX_NI_ENABLING_==_FEATURE_TICKTOCK_)
   return IsFeatureEnabled(ippCPUID_ADCOX)? cpMulAdx_BNU_school(pR, pA,nsA, pB,nsB)
                                          : cpMulAdc_BNU_school(pR, pA,nsA, pB,nsB);
#else
   return cpMulAdc_BNU_school(pR, pA,nsA, pB,nsB);
#endif
}


#define cpSqrAdc_BNU_school OWNAPI(cpSqrAdc_BNU_school)
   IPP_OWN_DECL (BNU_CHUNK_T, cpSqrAdc_BNU_school, (BNU_CHUNK_T * pR, const BNU_CHUNK_T * pA, cpSize nsA))
#define cpSqrAdx_BNU_school OWNAPI(cpSqrAdx_BNU_school)
   IPP_OWN_DECL (BNU_CHUNK_T, cpSqrAdx_BNU_school, (BNU_CHUNK_T * pR, const BNU_CHUNK_T * pA, cpSize nsA))

/*F*
//    Name: cpSqr_BNU_school
//
// Purpose: Square BigNum.
//
// Returns:
//    extension of result of square BigNum
//
// Parameters:
//    pA    source BigNum
//    pR    resultant BigNum
//
*F*/

__INLINE BNU_CHUNK_T cpSqr_BNU_school(BNU_CHUNK_T * pR, const BNU_CHUNK_T * pA, cpSize nsA)
{
#if(_ADCOX_NI_ENABLING_==_FEATURE_ON_)
   return cpSqrAdx_BNU_school(pR, pA,nsA);
#elif(_ADCOX_NI_ENABLING_==_FEATURE_TICKTOCK_)
   return IsFeatureEnabled(ippCPUID_ADCOX)? cpSqrAdx_BNU_school(pR, pA,nsA)
                                          : cpSqrAdc_BNU_school(pR, pA,nsA);
#else
   return cpSqrAdc_BNU_school(pR, pA,nsA);
#endif
}

#define cpGcd_BNU OWNAPI(cpGcd_BNU)
   IPP_OWN_DECL (BNU_CHUNK_T, cpGcd_BNU, (BNU_CHUNK_T a, BNU_CHUNK_T b))
#define cpModInv_BNU OWNAPI(cpModInv_BNU)
   IPP_OWN_DECL (int, cpModInv_BNU, (BNU_CHUNK_T* pInv, const BNU_CHUNK_T* pA, cpSize nsA, const BNU_CHUNK_T* pM, cpSize nsM, BNU_CHUNK_T* bufInv, BNU_CHUNK_T* bufA, BNU_CHUNK_T* bufM))

/*
// multiplication/squaring wrappers
*/
__INLINE BNU_CHUNK_T cpMul_BNU(BNU_CHUNK_T* pR,
                         const BNU_CHUNK_T* pA, cpSize nsA,
                         const BNU_CHUNK_T* pB, cpSize nsB,
                               BNU_CHUNK_T* pBuffer)
{
   IPP_UNREFERENCED_PARAMETER(pBuffer);
   return cpMul_BNU_school(pR, pA,nsA, pB,nsB);
}
__INLINE BNU_CHUNK_T cpSqr_BNU(BNU_CHUNK_T * pR,
                         const BNU_CHUNK_T * pA, cpSize nsA,
                               BNU_CHUNK_T* pBuffer)
{
   IPP_UNREFERENCED_PARAMETER(pBuffer);
   return cpSqr_BNU_school(pR, pA,nsA);
}

/*F*
//    Name: cpDiv_BNU
//
// Purpose: division/reduction BigNums.
//
// Returns:
//    size of result
//
// Parameters:
//    pA    source BigNum
//    pB    source BigNum
//    pQ    quotient BigNum
//    pnsQ  pointer to max size of Q
//    nsA   size of A
//    nsB   size of B
//
*F*/

__INLINE cpSize cpDiv_BNU(BNU_CHUNK_T* pQ, cpSize* pnsQ, BNU_CHUNK_T* pA, cpSize nsA, BNU_CHUNK_T* pB, cpSize nsB)
{
   int nsR = cpDiv_BNU32((Ipp32u*)pQ, pnsQ,
                         (Ipp32u*)pA, nsA*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)),
                         (Ipp32u*)pB, nsB*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));
   #if (BNU_CHUNK_BITS == BNU_CHUNK_64BIT)
   if(nsR&1) ((Ipp32u*)pA)[nsR] = 0;
   nsR = INTERNAL_BNU_LENGTH(nsR);
   if(pQ) {
      if(*pnsQ&1) ((Ipp32u*)pQ)[*pnsQ] = 0;
      *pnsQ = INTERNAL_BNU_LENGTH(*pnsQ);
   }
   #endif
   return nsR;
}

/*F*
//    Name: cpMod_BNU
//
// Purpose: reduction BigNums.
//
// Returns:
//    cpDiv_BNU(NULL,NULL, pX,nsX, pModulus, nsM)
//
// Parameters:
//    pX        source BigNum
//    pModulus  source BigNum
//    nsX       size of X
//    nsM       size of Modulus
//
*F*/

__INLINE cpSize cpMod_BNU(BNU_CHUNK_T* pX, cpSize nsX, BNU_CHUNK_T* pModulus, cpSize nsM)
{
   return cpDiv_BNU(NULL,NULL, pX,nsX, pModulus, nsM);
}

#endif /* _CP_BNU_ARITH_H */
