/*******************************************************************************
* Copyright 2018-2021 Intel Corporation
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
//     internal functions for GF(p^d) methods, if binomial generator
//     with Intel(R) Enhanced Privacy ID (Intel(R) EPID) 2.0 specific
//
*/
#include "owncp.h"

#include "pcpgfpxstuff.h"
#include "pcpgfpxmethod_com.h"

//tbcd: temporary excluded: #include <assert.h>

/*
// Intel(R) EPID 2.0 specific.
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
// Multiplication case: mul(a, xi) over GF(p^2),
// where:
//    a, belongs to GF(p^2)
//    xi belongs to GF(p^2), xi={2,1}
//
// The case is important in GF((p^2)^3) arithmetic for Intel(R) EPID 2.0.
//
*/
__INLINE BNU_CHUNK_T* cpFq2Mul_xi(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx)
{
   gsEngine* pGroundGFE = GFP_PARENT(pGFEx);
   mod_mul addF = GFP_METHOD(pGroundGFE)->add;
   mod_sub subF = GFP_METHOD(pGroundGFE)->sub;

   int termLen = GFP_FELEN(pGroundGFE);
   BNU_CHUNK_T* t0 = cpGFpGetPool(2, pGroundGFE);
   BNU_CHUNK_T* t1 = t0+termLen;

   const BNU_CHUNK_T* pA0 = pA;
   const BNU_CHUNK_T* pA1 = pA+termLen;
   BNU_CHUNK_T* pR0 = pR;
   BNU_CHUNK_T* pR1 = pR+termLen;

   //tbcd: temporary excluded: assert(NULL!=t0);
   addF(t0, pA0, pA0, pGroundGFE);
   addF(t1, pA0, pA1, pGroundGFE);
   subF(pR0, t0, pA1, pGroundGFE);
   addF(pR1, t1, pA1, pGroundGFE);

   cpGFpReleasePool(2, pGroundGFE);
   return pR;
}

/*
// Multiplication case: mul(a, g0) over GF(()),
// where:
//    a and g0 belongs to GF(()) - field is being extension
//
// The case is important in GF(()^d) arithmetic if constructed polynomial is generic binomial g(t) = t^d +g0.
//
*/
static BNU_CHUNK_T* cpGFpxMul_G0(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx)
{
   gsEngine* pGroundGFE = GFP_PARENT(pGFEx);
   BNU_CHUNK_T* pGFpolynomial = GFP_MODULUS(pGFEx); /* g(x) = t^d + g0 */
   return GFP_METHOD(pGroundGFE)->mul(pR, pA, pGFpolynomial, pGroundGFE);
}
