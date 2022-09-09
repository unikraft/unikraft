/*******************************************************************************
* Copyright 2010-2021 Intel Corporation
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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Internal EC over GF(p^m) basic Definitions & Function Prototypes
// 
//     Context:
//        gfec_point_double()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "gsscramble.h"



#if ( ECP_PROJECTIVE_COORD == JACOBIAN )
/*
// A = 4*x*y^2
// B = 3*x^2 + a*z^4
//
// x3 = -2*A + B^2
// y3 = -8y^4 +B*(A-x3)
// z3 = 2*y*z
//
// complexity: = 4s+4m (NIST's, SM2 curves)
//             = (EPID2 curve)
//             = 6s+4m (arbitrary curves)
*/
IPP_OWN_DEFN (void, gfec_point_double, (BNU_CHUNK_T* pRdata, const BNU_CHUNK_T* pPdata, IppsGFpECState* pEC))
{
   IppsGFpState* pGF = ECP_GFP(pEC);
   gsModEngine* pGFE = GFP_PMA(pGF);
   int elemLen = GFP_FELEN(pGFE);

   mod_add  add = GFP_METHOD(pGFE)->add;   /* gf add  */
   mod_sub  sub = GFP_METHOD(pGFE)->sub;   /* gf sub  */
   mod_div2 div2= GFP_METHOD(pGFE)->div2;  /* gf div2 */
   mod_mul2 mul2= GFP_METHOD(pGFE)->mul2;  /* gf mul2 */
   mod_mul3 mul3= GFP_METHOD(pGFE)->mul3;  /* gf mul3 */
   mod_mul  mul = GFP_METHOD(pGFE)->mul;   /* gf mul  */
   mod_sqr  sqr = GFP_METHOD(pGFE)->sqr;   /* gf sqr  */

   const BNU_CHUNK_T* pX = pPdata;
   const BNU_CHUNK_T* pY = pPdata+elemLen;
   const BNU_CHUNK_T* pZ = pPdata+2*+elemLen;

   BNU_CHUNK_T* rX = pRdata;
   BNU_CHUNK_T* rY = pRdata+elemLen;
   BNU_CHUNK_T* rZ = pRdata+2*elemLen;

   /* get temporary from top of EC point pool */
   BNU_CHUNK_T* U = pEC->pPool;
   BNU_CHUNK_T* M = U+elemLen;
   BNU_CHUNK_T* S = M+elemLen;

   mul2(S, pY, pGFE);            /* S = 2*Y */
   sqr(U, pZ, pGFE);             /* U = Z^2 */

   sqr(M, S, pGFE);              /* M = 4*Y^2 */
   mul(rZ, S, pZ, pGFE);         /* Zres = 2*Y*Z */

   sqr(rY, M, pGFE);             /* Yres = 16*Y^4 */

   mul(S, M, pX, pGFE);          /* S = 4*X*Y^2 */
   div2(rY, rY, pGFE);           /* Yres =  8*Y^4 */

   if(ECP_STD==ECP_SPECIFIC(pEC)) {
      add(M, pX, U, pGFE);       /* M = 3*(X^2-Z^4) */
      sub(U, pX, U, pGFE);
      mul(M, M, U, pGFE);
      mul3(M, M, pGFE);
   }
   else {
      sqr(M, pX, pGFE);          /* M = 3*X^2 */
      mul3(M, M, pGFE);
      if(ECP_EPID2!=ECP_SPECIFIC(pEC)) {
         sqr(U, U, pGFE);        /* M = 3*X^2+a*Z4 */
         mul(U, U, ECP_A(pEC), pGFE);
         add(M, M, U, pGFE);
      }
   }

   mul2(U, S, pGFE);             /* U = 8*X*Y^2 */
   sqr(rX, M, pGFE);             /* Xres = M^2 */
   sub(rX, rX, U, pGFE);         /* Xres = M^2-U */

   sub(S, S, rX, pGFE);          /* S = 4*X*Y^2-Xres */
   mul(S, S, M, pGFE);           /* S = M*(4*X*Y^2-Xres) */
   sub(rY, S, rY, pGFE);         /* Yres = M*(4*X*Y^2-Xres) -8*Y^4 */
}
#endif
