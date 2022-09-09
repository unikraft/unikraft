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
//        gfec_point_add()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcpmask_ct.h"


#if ( ECP_PROJECTIVE_COORD == JACOBIAN )
/*
// S1 = y1*z2^3
// S2 = y2*z1^3
//
// U1 = x1*z2^2
// U2 = x2*z1^2

//  R = S2-S1
//  H = U2-U1
//
//  x3 = -H^3 -2*U1*H^2 +R2
//  y3 = -S1*H^3 +R*(U1*H^2 -x3)
//  z3 = z1*z2*H
//
// complexity = 4s+12m
*/

IPP_OWN_DEFN (void, gfec_point_add, (BNU_CHUNK_T* pRdata, const BNU_CHUNK_T* pPdata, const BNU_CHUNK_T* pQdata, IppsGFpECState* pEC))
{
   IppsGFpState* pGF = ECP_GFP(pEC);
   gsModEngine* pGFE = GFP_PMA(pGF);
   int elemLen = GFP_FELEN(pGFE);

   mod_sub  sub = GFP_METHOD(pGFE)->sub;   /* gf sub  */
   mod_mul2 mul2= GFP_METHOD(pGFE)->mul2;  /* gf mul2 */
   mod_mul  mul = GFP_METHOD(pGFE)->mul;   /* gf mul  */
   mod_sqr  sqr = GFP_METHOD(pGFE)->sqr;   /* gf sqr  */

   /* coordinates of P */
   const BNU_CHUNK_T* px1 = pPdata;
   const BNU_CHUNK_T* py1 = pPdata+elemLen;
   const BNU_CHUNK_T* pz1 = pPdata+2*elemLen;

   /* coordinates of Q */
   const BNU_CHUNK_T* px2 = pQdata;
   const BNU_CHUNK_T* py2 = pQdata+elemLen;
   const BNU_CHUNK_T* pz2 = pQdata+2*elemLen;

   BNU_CHUNK_T inftyP = GFPE_IS_ZERO_CT(pz1, elemLen);
   BNU_CHUNK_T inftyQ = GFPE_IS_ZERO_CT(pz2, elemLen);

   /* get temporary from top of EC point pool */
   BNU_CHUNK_T* U1 = pEC->pPool;
   BNU_CHUNK_T* U2 = U1 + elemLen;
   BNU_CHUNK_T* S1 = U2 + elemLen;
   BNU_CHUNK_T* S2 = S1 + elemLen;
   BNU_CHUNK_T* H  = S2 + elemLen;
   BNU_CHUNK_T* R  = H  + elemLen;

   BNU_CHUNK_T* pRx = R  + elemLen; /* temporary result */
   BNU_CHUNK_T* pRy = pRx+ elemLen;
   BNU_CHUNK_T* pRz = pRy+ elemLen;

   mul(S1, py1, pz2, pGFE);       // S1 = Y1*Z2
   sqr(U1, pz2, pGFE);            // U1 = Z2^2

   mul(S2, py2, pz1, pGFE);       // S2 = Y2*Z1
   sqr(U2, pz1, pGFE);            // U2 = Z1^2

   mul(S1, S1, U1, pGFE);         // S1 = Y1*Z2^3
   mul(S2, S2, U2, pGFE);         // S2 = Y2*Z1^3

   mul(U1, px1, U1, pGFE);        // U1 = X1*Z2^2
   mul(U2, px2, U2, pGFE);        // U2 = X2*Z1^2

   sub(R, S2, S1, pGFE);          // R = S2-S1
   sub(H, U2, U1, pGFE);          // H = U2-U1

   {
      BNU_CHUNK_T mask_zeroH = GFPE_IS_ZERO_CT(H, elemLen);
      BNU_CHUNK_T mask = mask_zeroH & ~inftyP & ~inftyQ;
      if(mask) {
         if( GFPE_IS_ZERO_CT(R, elemLen) )
            gfec_point_double(pRdata, pPdata, pEC);
         else
            cpGFpElementPad(pRdata, 3*elemLen, 0);
         return;
      }
   }

   mul(pRz, pz1, pz2, pGFE);      // Z3 = Z1*Z2
   sqr(U2, H, pGFE);              // U2 = H^2
   mul(pRz, pRz, H, pGFE);        // Z3 = (Z1*Z2)*H
   sqr(S2, R, pGFE);              // S2 = R^2
   mul(H, H, U2, pGFE);           // H = H^3

   mul(U1, U1, U2, pGFE);         // U1 = U1*H^2
   sub(pRx, S2, H, pGFE);         // X3 = R^2 - H^3
   mul2(U2, U1, pGFE);            // U2 = 2*U1*H^2
   mul(S1, S1, H, pGFE);          // S1 = S1*H^3
   sub(pRx, pRx, U2, pGFE);       // X3 = (R^2 - H^3) -2*U1*H^2

   sub(pRy, U1, pRx, pGFE);       // Y3 = R*(U1*H^2 - X3) -S1*H^3
   mul(pRy, pRy, R, pGFE);
   sub(pRy, pRy, S1, pGFE);

   cpMaskedReplace_ct(pRx, px2, elemLen*3, inftyP);
   cpMaskedReplace_ct(pRx, px1, elemLen*3, inftyQ);

   cpGFpElementCopy(pRdata, pRx, 3*elemLen);
}
#endif

