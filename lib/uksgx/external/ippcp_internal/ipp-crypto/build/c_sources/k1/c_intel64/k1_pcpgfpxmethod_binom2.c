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

#include "pcpgfpxmethod_binom_mulc.h"
#include "pcpgfpxmethod_com.h"

//tbcd: temporary excluded: #include <assert.h>

/*
// Multiplication in GF(p^2), if field polynomial: g(x) = x^2 + beta  => binominal
*/
IPP_OWN_DEFN (static BNU_CHUNK_T*, cpGFpxMul_p2_binom, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFEx))
{
   gsEngine* pGroundGFE = GFP_PARENT(pGFEx);
   int groundElemLen = GFP_FELEN(pGroundGFE);

   mod_mul mulF = GFP_METHOD(pGroundGFE)->mul;
   mod_add addF = GFP_METHOD(pGroundGFE)->add;
   mod_sub subF = GFP_METHOD(pGroundGFE)->sub;

   const BNU_CHUNK_T* pA0 = pA;
   const BNU_CHUNK_T* pA1 = pA+groundElemLen;

   const BNU_CHUNK_T* pB0 = pB;
   const BNU_CHUNK_T* pB1 = pB+groundElemLen;

   BNU_CHUNK_T* pR0 = pR;
   BNU_CHUNK_T* pR1 = pR+groundElemLen;

   BNU_CHUNK_T* t0 = cpGFpGetPool(4, pGroundGFE);
   BNU_CHUNK_T* t1 = t0+groundElemLen;
   BNU_CHUNK_T* t2 = t1+groundElemLen;
   BNU_CHUNK_T* t3 = t2+groundElemLen;
   //tbcd: temporary excluded: assert(NULL!=t0);

   #if defined GS_DBG
   BNU_CHUNK_T* arg0 = cpGFpGetPool(1, pGroundGFE);
   BNU_CHUNK_T* arg1 = cpGFpGetPool(1, pGroundGFE);
   #endif
   #if defined GS_DBG
   cpGFpxGet(arg0, groundElemLen, pA0, pGroundGFE);
   cpGFpxGet(arg1, groundElemLen, pB0, pGroundGFE);
   #endif

   mulF(t0, pA0, pB0, pGroundGFE);    /* t0 = a[0]*b[0] */

   #if defined GS_DBG
   cpGFpxGet(arg0, groundElemLen, pA1, pGroundGFE);
   cpGFpxGet(arg1, groundElemLen, pB1, pGroundGFE);
   #endif

   mulF(t1, pA1, pB1, pGroundGFE);    /* t1 = a[1]*b[1] */
   addF(t2, pA0, pA1, pGroundGFE);    /* t2 = a[0]+a[1] */
   addF(t3, pB0, pB1, pGroundGFE);    /* t3 = b[0]+b[1] */

   #if defined GS_DBG
   cpGFpxGet(arg0, groundElemLen, t2, pGroundGFE);
   cpGFpxGet(arg1, groundElemLen, t3, pGroundGFE);
   #endif

   mulF(pR1, t2,  t3, pGroundGFE);    /* r[1] = (a[0]+a[1]) * (b[0]+b[1]) */
   subF(pR1, pR1, t0, pGroundGFE);    /* r[1] -= a[0]*b[0]) + a[1]*b[1] */
   subF(pR1, pR1, t1, pGroundGFE);

   cpGFpxMul_G0(t1, t1, pGFEx);
   subF(pR0, t0, t1, pGroundGFE);

   #if defined GS_DBG
   cpGFpReleasePool(2, pGroundGFE);
   #endif

   cpGFpReleasePool(4, pGroundGFE);
   return pR;
}

/*
// Squaring in GF(p^2), if field polynomial: g(x) = x^2 + beta  => binominal
*/
IPP_OWN_DEFN (static BNU_CHUNK_T*, cpGFpxSqr_p2_binom, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx))
{
   gsEngine* pGroundGFE = GFP_PARENT(pGFEx);
   int groundElemLen = GFP_FELEN(pGroundGFE);

   mod_mul mulF = GFP_METHOD(pGroundGFE)->mul;
   mod_sqr sqrF = GFP_METHOD(pGroundGFE)->sqr;
   mod_add addF = GFP_METHOD(pGroundGFE)->add;
   mod_sub subF = GFP_METHOD(pGroundGFE)->sub;

   const BNU_CHUNK_T* pA0 = pA;
   const BNU_CHUNK_T* pA1 = pA+groundElemLen;

   BNU_CHUNK_T* pR0 = pR;
   BNU_CHUNK_T* pR1 = pR+groundElemLen;

   BNU_CHUNK_T* t0 = cpGFpGetPool(3, pGroundGFE);
   BNU_CHUNK_T* t1 = t0+groundElemLen;
   BNU_CHUNK_T* u0 = t1+groundElemLen;
   //tbcd: temporary excluded: assert(NULL!=t0);

   #if defined GS_DBG
   BNU_CHUNK_T* arg0 = cpGFpGetPool(1, pGroundGFE);
   BNU_CHUNK_T* arg1 = cpGFpGetPool(1, pGroundGFE);
   #endif
   #if defined GS_DBG
   cpGFpxGet(arg0, groundElemLen, pA0, pGroundGFE);
   cpGFpxGet(arg1, groundElemLen, pA1, pGroundGFE);
   #endif

   mulF(u0, pA0, pA1, pGroundGFE); /* u0 = a[0]*a[1] */
   sqrF(t0, pA0, pGroundGFE);      /* t0 = a[0]*a[0] */
   sqrF(t1, pA1, pGroundGFE);      /* t1 = a[1]*a[1] */
   cpGFpxMul_G0(t1, t1, pGFEx);
   subF(pR0, t0, t1, pGroundGFE);
   addF(pR1, u0, u0, pGroundGFE);  /* r[1] = 2*a[0]*a[1] */

   #if defined GS_DBG
   cpGFpReleasePool(2, pGroundGFE);
   #endif

   cpGFpReleasePool(3, pGroundGFE);
   return pR;
}

/*
// return specific polynomi alarith methods
// polynomial - deg 2 binomial
*/
static gsModMethod* gsPolyArith_binom2 (void)
{
   static gsModMethod m = {
      cpGFpxEncode_com,
      cpGFpxDecode_com,
      cpGFpxMul_p2_binom,
      cpGFpxSqr_p2_binom,
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
//                   g(x) = x^2 - a0, a0 from GF(p)
//
//
*F*/

IPPFUN( const IppsGFpMethod*, ippsGFpxMethod_binom2, (void) )
{
   static IppsGFpMethod method = {
      cpID_Binom,
      2,
      NULL,
      NULL
   };
   method.arith = gsPolyArith_binom2();
   return &method;
}
