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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     GF(p^d) methods, if binomial generator
//
*/
#include "owncp.h"

#include "pcpgfpxstuff.h"
#include "pcpgfpxmethod_com.h"

//tbcd: temporary excluded: #include <assert.h>

/*
// Multiplication in GF(p^d), if field polynomial: g(x) = x^d + beta  => binominal
*/
IPP_OWN_DEFN (static BNU_CHUNK_T*, cpGFpxMul_pd_binom, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFEx))
{
   BNU_CHUNK_T* pGFpolynomial = GFP_MODULUS(pGFEx);
   int deg = GFP_EXTDEGREE(pGFEx);
   int elemLen= GFP_FELEN(pGFEx);
   int groundElemLen = GFP_FELEN(GFP_PARENT(pGFEx));
   int d;

   BNU_CHUNK_T* R = cpGFpGetPool(4, pGFEx);
   BNU_CHUNK_T* X = R+elemLen;
   BNU_CHUNK_T* T0= X+elemLen;
   BNU_CHUNK_T* T1= T0+elemLen;
   //tbcd: temporary excluded: assert(NULL!=R);

   /* T0 = A * beta */
   cpGFpxMul_GFE(T0, pA, pGFpolynomial, pGFEx);
   /* T1 = A */
   cpGFpElementCopy(T1, pA, elemLen);

   /* R = A * B[0] */
   cpGFpxMul_GFE(R, pA, pB, pGFEx);

   /* R += (A*B[d]) mod g() */
   for(d=1; d<deg; d++) {
      cpGFpxMul_GFE(X, GFPX_IDX_ELEMENT(T0, deg-d, groundElemLen), GFPX_IDX_ELEMENT(pB, d, groundElemLen),  pGFEx);
      GFP_METHOD(pGFEx)->add(R, R, X, pGFEx);
   }
   cpGFpElementCopy(pR, R, elemLen);

   cpGFpReleasePool(4, pGFEx);
   return pR;
}

/*
// Squaring in GF(p^d), if field polynomial: g(x) = x^d + beta  => binominal
*/
IPP_OWN_DEFN (static BNU_CHUNK_T*, cpGFpxSqr_pd_binom, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx))
{
   #if defined(__INTEL_COMPILER)
   #pragma noinline
   #endif
   return cpGFpxMul_pd_binom(pR, pA, pA, pGFEx);
}

/*
// return specific polynomial arith methods
// polynomial - general binomial
*/
static gsModMethod* gsPolyArith_binom (void)
{
   static gsModMethod m = {
      cpGFpxEncode_com,
      cpGFpxDecode_com,
      cpGFpxMul_pd_binom,
      cpGFpxSqr_pd_binom,
      NULL,
      cpGFpxAdd_com,
      cpGFpxSub_com,
      cpGFpxNeg_com,
      cpGFpxDiv2_com,
      cpGFpxMul2_com,
      cpGFpxMul3_com,
      //cpGFpxInv
   };
   return &m;
}

/*F*
// Name: ippsGFpxMethod_binom2
//
// Purpose: Returns a reference to the implementation of arithmetic operations over GF(pd).
//
// Returns:          pointer to a structure containing
//                   an implementation of arithmetic operations over GF(pd)
//                   g(x) = x^d - a0, a0 from GF(p)
//
//
*F*/

IPPFUN( const IppsGFpMethod*, ippsGFpxMethod_binom, (void) )
{
   static IppsGFpMethod method = {
      cpID_Binom,
      0,
      NULL,
      NULL
   };
   method.arith = gsPolyArith_binom();
   return &method;
}
