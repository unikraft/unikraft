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
// Multiplication in GF(p^3), if field polynomial: g(x) = x^3 + beta  => binominal
*/
IPP_OWN_DEFN (static BNU_CHUNK_T*, cpGFpxMul_p3_binom, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFEx))
{
   gsEngine* pGroundGFE = GFP_PARENT(pGFEx);
   int groundElemLen = GFP_FELEN(pGroundGFE);

   mod_mul mulF = GFP_METHOD(pGroundGFE)->mul;
   mod_add addF = GFP_METHOD(pGroundGFE)->add;
   mod_sub subF = GFP_METHOD(pGroundGFE)->sub;

   const BNU_CHUNK_T* pA0 = pA;
   const BNU_CHUNK_T* pA1 = pA+groundElemLen;
   const BNU_CHUNK_T* pA2 = pA+groundElemLen*2;

   const BNU_CHUNK_T* pB0 = pB;
   const BNU_CHUNK_T* pB1 = pB+groundElemLen;
   const BNU_CHUNK_T* pB2 = pB+groundElemLen*2;

   BNU_CHUNK_T* pR0 = pR;
   BNU_CHUNK_T* pR1 = pR+groundElemLen;
   BNU_CHUNK_T* pR2 = pR+groundElemLen*2;

   BNU_CHUNK_T* t0 = cpGFpGetPool(6, pGroundGFE);
   BNU_CHUNK_T* t1 = t0+groundElemLen;
   BNU_CHUNK_T* t2 = t1+groundElemLen;
   BNU_CHUNK_T* u0 = t2+groundElemLen;
   BNU_CHUNK_T* u1 = u0+groundElemLen;
   BNU_CHUNK_T* u2 = u1+groundElemLen;
   //tbcd: temporary excluded: assert(NULL!=t0);

   addF(u0 ,pA0, pA1, pGroundGFE);    /* u0 = a[0]+a[1] */
   addF(t0 ,pB0, pB1, pGroundGFE);    /* t0 = b[0]+b[1] */
   mulF(u0, u0,  t0,  pGroundGFE);    /* u0 = (a[0]+a[1])*(b[0]+b[1]) */
   mulF(t0, pA0, pB0, pGroundGFE);    /* t0 = a[0]*b[0] */

   addF(u1 ,pA1, pA2, pGroundGFE);    /* u1 = a[1]+a[2] */
   addF(t1 ,pB1, pB2, pGroundGFE);    /* t1 = b[1]+b[2] */
   mulF(u1, u1,  t1,  pGroundGFE);    /* u1 = (a[1]+a[2])*(b[1]+b[2]) */
   mulF(t1, pA1, pB1, pGroundGFE);    /* t1 = a[1]*b[1] */

   addF(u2 ,pA2, pA0, pGroundGFE);    /* u2 = a[2]+a[0] */
   addF(t2 ,pB2, pB0, pGroundGFE);    /* t2 = b[2]+b[0] */
   mulF(u2, u2,  t2,  pGroundGFE);    /* u2 = (a[2]+a[0])*(b[2]+b[0]) */
   mulF(t2, pA2, pB2, pGroundGFE);    /* t2 = a[2]*b[2] */

   subF(u0, u0,  t0,  pGroundGFE);    /* u0 = a[0]*b[1]+a[1]*b[0] */
   subF(u0, u0,  t1,  pGroundGFE);
   subF(u1, u1,  t1,  pGroundGFE);    /* u1 = a[1]*b[2]+a[2]*b[1] */
   subF(u1, u1,  t2,  pGroundGFE);
   subF(u2, u2,  t2,  pGroundGFE);    /* u2 = a[2]*b[0]+a[0]*b[2] */
   subF(u2, u2,  t0,  pGroundGFE);

   cpGFpxMul_G0(u1, u1, pGFEx); /* u1 = (a[1]*b[2]+a[2]*b[1]) * beta */
   cpGFpxMul_G0(t2, t2, pGFEx); /* t2 = a[2]*b[2] * beta */

   subF(pR0, t0, u1,  pGroundGFE);    /* r[0] = a[0]*b[0] - (a[2]*b[1]+a[1]*b[2])*beta */
   subF(pR1, u0, t2,  pGroundGFE);    /* r[1] = a[1]*b[0] + a[0]*b[1] - a[2]*b[2]*beta */

   addF(pR2, u2, t1,  pGroundGFE);     /* r[2] = a[2]*b[0] + a[1]*b[1] + a[0]*b[2] */

   cpGFpReleasePool(6, pGroundGFE);
   return pR;
}

/*
// Squaring in GF(p^3), if field polynomial: g(x) = x^3 + beta  => binominal
*/
IPP_OWN_DEFN (static BNU_CHUNK_T*, cpGFpxSqr_p3_binom, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx))
{
   gsEngine* pGroundGFE = GFP_PARENT(pGFEx);
   int groundElemLen = GFP_FELEN(pGroundGFE);

   mod_mul mulF = GFP_METHOD(pGroundGFE)->mul;
   mod_sqr sqrF = GFP_METHOD(pGroundGFE)->sqr;
   mod_add addF = GFP_METHOD(pGroundGFE)->add;
   mod_sub subF = GFP_METHOD(pGroundGFE)->sub;

   const BNU_CHUNK_T* pA0 = pA;
   const BNU_CHUNK_T* pA1 = pA+groundElemLen;
   const BNU_CHUNK_T* pA2 = pA+groundElemLen*2;

   BNU_CHUNK_T* pR0 = pR;
   BNU_CHUNK_T* pR1 = pR+groundElemLen;
   BNU_CHUNK_T* pR2 = pR+groundElemLen*2;

   BNU_CHUNK_T* s0 = cpGFpGetPool(5, pGroundGFE);
   BNU_CHUNK_T* s1 = s0+groundElemLen;
   BNU_CHUNK_T* s2 = s1+groundElemLen;
   BNU_CHUNK_T* s3 = s2+groundElemLen;
   BNU_CHUNK_T* s4 = s3+groundElemLen;
   //tbcd: temporary excluded: assert(NULL!=s0);

   addF(s2, pA0, pA2, pGroundGFE);
   subF(s2,  s2, pA1, pGroundGFE);
   sqrF(s2,  s2, pGroundGFE);
   sqrF(s0, pA0, pGroundGFE);
   sqrF(s4, pA2, pGroundGFE);
   mulF(s1, pA0, pA1, pGroundGFE);
   mulF(s3, pA1, pA2, pGroundGFE);
   addF(s1,  s1,  s1, pGroundGFE);
   addF(s3,  s3,  s3, pGroundGFE);

   addF(pR2,  s1, s2, pGroundGFE);
   addF(pR2, pR2, s3, pGroundGFE);
   subF(pR2, pR2, s0, pGroundGFE);
   subF(pR2, pR2, s4, pGroundGFE);

   cpGFpxMul_G0(s4, s4, pGFEx);
   subF(pR1, s1,  s4, pGroundGFE);

   cpGFpxMul_G0(s3, s3, pGFEx);
   subF(pR0, s0,  s3, pGroundGFE);

   cpGFpReleasePool(5, pGroundGFE);
   return pR;
}


/*
// return specific polynomi alarith methods
// polynomial - deg 3 binomial
*/
static gsModMethod* gsPolyArith_binom3 (void)
{
   static gsModMethod m = {
      cpGFpxEncode_com,
      cpGFpxDecode_com,
      cpGFpxMul_p3_binom,
      cpGFpxSqr_p3_binom,
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
//                   g(x) = x^3 - a0, a0 from GF(p)
//
//
*F*/
IPPFUN( const IppsGFpMethod*, ippsGFpxMethod_binom3, (void) )
{
   static IppsGFpMethod method = {
      cpID_Binom,
      3,
      NULL,
      NULL
   };
   method.arith = gsPolyArith_binom3();
   return &method;
}
