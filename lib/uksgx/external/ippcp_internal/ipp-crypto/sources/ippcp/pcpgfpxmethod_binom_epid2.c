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
#include "pcpgfpxmethod_binom_epid2.h"

//tbcd: temporary excluded: #include <assert.h>

/*
// Intel(R) Enhanced Privacy ID (Intel(R) EPID) 2.0 specific.
//
// Intel(R) EPID 2.0 uses the following finite field hierarchy:
//
// 1) prime field GF(p),
//    p = 0xFFFFFFFFFFFCF0CD46E5F25EEE71A49F0CDC65FB12980A82D3292DDBAED33013
//
// 2) 2-degree extension of GF(p): GF(p^2) == GF(p)[x]/g(x), g(x) = x^2 -beta,
//    beta =-1 mod p, so "beta" represents as {1}
//
// 3) 3-degree extension of GF(p^2) ~ GF(p^6): GF((p^2)^3) == GF(p)[v]/g(v), g(v) = v^3 -xi,
//    xi belongs GF(p^2), xi=x+2, so "xi" represents as {2,1} ---- "2" is low- and "1" is high-order coefficients
//
// 4) 2-degree extension of GF((p^2)^3) ~ GF(p^12): GF(((p^2)^3)^2) == GF(p)[w]/g(w), g(w) = w^2 -vi,
//    psi belongs GF((p^2)^3), vi=0*v^2 +1*v +0, so "vi" represents as {0,1,0}---- "0", '1" and "0" are low-, middle- and high-order coefficients
//
// See representations in t_gfpparam.cpp
//
*/

/*
// Multiplication case: mul(a, vi) over GF((p^2)^3),
// where:
//    a, belongs to GF((p^2)^3)
//    xi belongs to GF((p^2)^3), vi={0,1,0}
//
// The case is important in GF(((p^2)^3)^2) arithmetic for Intel(R) EPID 2.0.
//
*/
__INLINE BNU_CHUNK_T* cpFq6Mul_vi(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx)
{
   gsEngine* pGroundGFE = GFP_PARENT(pGFEx);
   int termLen = GFP_FELEN(pGroundGFE);

   const BNU_CHUNK_T* pA0 = pA;
   const BNU_CHUNK_T* pA1 = pA+termLen;
   const BNU_CHUNK_T* pA2 = pA+termLen*2;
   BNU_CHUNK_T* pR0 = pR;
   BNU_CHUNK_T* pR1 = pR+termLen;
   BNU_CHUNK_T* pR2 = pR+termLen*2;

   BNU_CHUNK_T* t = cpGFpGetPool(1, pGroundGFE);
   //tbcd: temporary excluded: assert(NULL!=t);

   cpFq2Mul_xi(t, pA2, pGroundGFE);
   cpGFpElementCopy(pR2, pA1, termLen);
   cpGFpElementCopy(pR1, pA0, termLen);
   cpGFpElementCopy(pR0, t, termLen);

   cpGFpReleasePool(1, pGroundGFE);

   return pR;
}

/*
// Intel(R) EPID 2.0 specific
// ~~~~~~~~~~~~~~~
//
// Multiplication over GF(p^2)
//    - field polynomial: g(x) = x^2 - beta  => binominal with specific value of "beta"
//    - beta = p-1
//
// Multiplication over GF(((p^2)^3)^2)    ~ GF(p^12)
//    - field polynomial: g(w) = w^2 - vi   => binominal with specific value of "vi"
//    - vi = 0*v^2 + 1*v + 0 - i.e vi={0,1,0} belongs to GF((p^2)^3)
*/
IPP_OWN_DEFN (static BNU_CHUNK_T*, cpGFpxMul_p2_binom_epid2, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFEx))
{
   gsEngine* pGroundGFE = GFP_PARENT(pGFEx);
   mod_mul mulF = GFP_METHOD(pGroundGFE)->mul;
   mod_add addF = GFP_METHOD(pGroundGFE)->add;
   mod_sub subF = GFP_METHOD(pGroundGFE)->sub;

   int groundElemLen = GFP_FELEN(pGroundGFE);

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

   mulF(t0, pA0, pB0, pGroundGFE);    /* t0 = a[0]*b[0] */
   mulF(t1, pA1, pB1, pGroundGFE);    /* t1 = a[1]*b[1] */
   addF(t2, pA0, pA1, pGroundGFE);    /* t2 = a[0]+a[1] */
   addF(t3, pB0, pB1, pGroundGFE);    /* t3 = b[0]+b[1] */

   mulF(pR1, t2,  t3, pGroundGFE);    /* r[1] = (a[0]+a[1]) * (b[0]+b[1]) */
   subF(pR1, pR1, t0, pGroundGFE);    /* r[1] -= a[0]*b[0]) + a[1]*b[1] */
   subF(pR1, pR1, t1, pGroundGFE);

   /* Intel(R) EPID 2.0 specific */
   {
      int basicExtDegree = cpGFpBasicDegreeExtension(pGFEx);

      /* deal with GF(p^2) */
      if(basicExtDegree==2) {
         subF(pR0, t0, t1, pGroundGFE);
      }
      /* deal with GF(p^6^2) */
      else if(basicExtDegree==12) {
         cpFq6Mul_vi(t1, t1, pGroundGFE);
         addF(pR0, t0, t1, pGroundGFE);
      }
      /* deal with GF(p^x^2) - it's not Intel(R) EPID 2.0 case, just a case */
      else {
         cpGFpxMul_G0(t1, t1, pGFEx);
         subF(pR0, t0, t1, pGroundGFE);
      }
   }

   cpGFpReleasePool(4, pGroundGFE);
   return pR;
}

/*
// Intel(R) EPID 2.0 specific
// ~~~~~~~~~~~~~~~
//
// Squaring over GF(p^2)
//    - field polynomial: g(x) = x^2 - beta  => binominal with specific value of "beta"
//    - beta = p-1
//
// Squaring in GF(((p^2)^3)^2)      ~ GF(p^12)
//    - field polynomial: g(w) = w^2 - vi   => binominal with specific value of "vi"
//    - vi = 0*v^2 + 1*v + 0 - i.e vi={0,1,0} belongs to GF((p^2)^3)
*/
IPP_OWN_DEFN (static BNU_CHUNK_T*, cpGFpxSqr_p2_binom_epid2, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx))
{
   gsEngine* pGroundGFE = GFP_PARENT(pGFEx);
   mod_mul mulF = GFP_METHOD(pGroundGFE)->mul;
   mod_sqr sqrF = GFP_METHOD(pGroundGFE)->sqr;
   mod_add addF = GFP_METHOD(pGroundGFE)->add;
   mod_sub subF = GFP_METHOD(pGroundGFE)->sub;

   int groundElemLen = GFP_FELEN(pGroundGFE);

   const BNU_CHUNK_T* pA0 = pA;
   const BNU_CHUNK_T* pA1 = pA+groundElemLen;

   BNU_CHUNK_T* pR0 = pR;
   BNU_CHUNK_T* pR1 = pR+groundElemLen;

   BNU_CHUNK_T* t0 = cpGFpGetPool(3, pGroundGFE);
   BNU_CHUNK_T* t1 = t0+groundElemLen;
   BNU_CHUNK_T* u0 = t1+groundElemLen;
   //tbcd: temporary excluded: assert(NULL!=t0);

   mulF(u0, pA0, pA1, pGroundGFE); /* u0 = a[0]*a[1] */

   /* Intel(R) EPID 2.0 specific */
   {
      int basicExtDegree = cpGFpBasicDegreeExtension(pGFEx);

      /* deal with GF(p^2) */
      if(basicExtDegree==2) {
         addF(t0, pA0, pA1, pGroundGFE);
         subF(t1, pA0, pA1, pGroundGFE);
         mulF(pR0, t0, t1,  pGroundGFE);
         addF(pR1, u0, u0,  pGroundGFE);  /* r[1] = 2*a[0]*a[1] */
      }
      /* deal with GF(p^6^2) */
      else if(basicExtDegree==12) {
         subF(t0, pA0, pA1, pGroundGFE);
         cpFq6Mul_vi(t1, pA1, pGroundGFE);
         subF(t1, pA0, t1, pGroundGFE);
         mulF(t0, t0, t1, pGroundGFE);
         addF(t0, t0, u0, pGroundGFE);
         cpFq6Mul_vi(t1, u0, pGroundGFE);
         addF(pR0, t0, t1, pGroundGFE);
         addF(pR1, u0, u0, pGroundGFE);
      }
      /* just a case */
      else {
         sqrF(t0, pA0, pGroundGFE);      /* t0 = a[0]*a[0] */
         sqrF(t1, pA1, pGroundGFE);      /* t1 = a[1]*a[1] */
         cpGFpxMul_G0(t1, t1, pGFEx);
         subF(pR0, t0, t1, pGroundGFE);
         addF(pR1, u0, u0, pGroundGFE);  /* r[1] = 2*a[0]*a[1] */
      }
   }

   cpGFpReleasePool(3, pGroundGFE);
   return pR;
}

/*
// return specific polynomi alarith methods
// polynomial - deg 2 binomial (Intel(R) EPID 2.0)
*/
static gsModMethod* gsPolyArith_binom2_epid2 (void)
{
   static gsModMethod m = {
      cpGFpxEncode_com,
      cpGFpxDecode_com,
      cpGFpxMul_p2_binom_epid2,
      cpGFpxSqr_p2_binom_epid2,
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
// Name: ippsGFpxMethod_binom2_epid2
//
// Purpose: Returns a reference to the implementation of arithmetic operations over GF(pd).
//
// Returns:          pointer to a structure containing 
//                   an implementation of arithmetic operations over GF(pd)
//                   g(x) = x^2 - a0, a0 from GF(q), a0 = 1
//                   g(w) = w^2 - V0, v0 from GF((q^2)^3), V0 = 0*s^2 + v + 0
//
//
*F*/

IPPFUN( const IppsGFpMethod*, ippsGFpxMethod_binom2_epid2, (void) )
{
   static IppsGFpMethod method = {
      cpID_Binom2_epid20,
      2,
      NULL,
      NULL
   };
   method.arith = gsPolyArith_binom2_epid2();
   return &method;
}

