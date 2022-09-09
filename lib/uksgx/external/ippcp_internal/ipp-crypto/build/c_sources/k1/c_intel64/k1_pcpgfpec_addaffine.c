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
//        gfec_affine_point_add()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcpmask_ct.h"


#if ( ECP_PROJECTIVE_COORD == JACOBIAN )
/*
// complexity = 3s+8m
*/
IPP_OWN_DEFN (void, gfec_affine_point_add, (BNU_CHUNK_T* pRdata, const BNU_CHUNK_T* pPdata, const BNU_CHUNK_T* pAdata, IppsGFpECState* pEC))
{
   IppsGFpState* pGF = ECP_GFP(pEC);
   gsModEngine* pGFE = GFP_PMA(pGF);
   int elemLen = GFP_FELEN(pGFE);

   mod_sub  sub = GFP_METHOD(pGFE)->sub;   /* gf sub  */
   mod_mul2 mul2= GFP_METHOD(pGFE)->mul2;  /* gf mul2 */
   mod_mul  mul = GFP_METHOD(pGFE)->mul;   /* gf mul  */
   mod_sqr  sqr = GFP_METHOD(pGFE)->sqr;   /* gf sqr  */

   BNU_CHUNK_T* mont1 = GFP_MNT_R(pGFE);

   /* coordinates of projective P point */
   const BNU_CHUNK_T* px = pPdata;              /* x1 */
   const BNU_CHUNK_T* py = pPdata+elemLen;      /* y1 */
   const BNU_CHUNK_T* pz = pPdata+2*elemLen;    /* z1 */

   /* coordinates of affine A point, az==mont(1) */
   const BNU_CHUNK_T* ax = pAdata;              /* x2 */
   const BNU_CHUNK_T* ay = pAdata+elemLen;      /* y2 */

   BNU_CHUNK_T inftyP = GFPE_IS_ZERO_CT(px, elemLen) & GFPE_IS_ZERO_CT(py, elemLen);
   BNU_CHUNK_T inftyA = GFPE_IS_ZERO_CT(ax, elemLen) & GFPE_IS_ZERO_CT(ay, elemLen);

   /* get temporary from top of EC point pool */
   BNU_CHUNK_T* U2 = pEC->pPool;
   BNU_CHUNK_T* S2 = U2 + elemLen;
   BNU_CHUNK_T* H  = S2 + elemLen;
   BNU_CHUNK_T* R  = H  + elemLen;

   BNU_CHUNK_T* pRx = R  + elemLen; /* temporary result */
   BNU_CHUNK_T* pRy = pRx+ elemLen;
   BNU_CHUNK_T* pRz = pRy+ elemLen;

   sqr(R, pz, pGFE);             // R = Z1^2
   mul(S2, ay, pz, pGFE);        // S2 = Y2*Z1
   mul(U2, ax, R, pGFE);         // U2 = X2*Z1^2
   mul(S2, S2, R, pGFE);         // S2 = Y2*Z1^3

   sub(H, U2, px, pGFE);         // H = U2-X1
   sub(R, S2, py, pGFE);         // R = S2-Y1

   mul(pRz, H, pz, pGFE);        // Z3 = H*Z1

   sqr(U2, H, pGFE);             // U2 = H^2
   sqr(S2, R, pGFE);             // S2 = R^2
   mul(H, H, U2, pGFE);          // H = H^3

   mul(U2, U2, px, pGFE);        // U2 = X1*H^2

   mul(pRy, H, py, pGFE);        // T = Y1*H^3

   mul2(pRx, U2, pGFE);          // X3 = 2*X1*H^2
   sub(pRx, S2, pRx, pGFE);      // X3 = R^2 - 2*X1*H^2
   sub(pRx, pRx, H, pGFE);       // X3 = R^2 - 2*X1*H^2 -H^3

   sub(U2, U2, pRx, pGFE);       // U2 = X1*H^2 - X3
   mul(U2, U2, R, pGFE);         // U2 = R*(X1*H^2 - X3)
   sub(pRy, U2, pRy, pGFE);      // Y3 = -Y1*H^3 + R*(X1*H^2 - X3)

   cpMaskedReplace_ct(pRx, ax, elemLen, inftyP);
   cpMaskedReplace_ct(pRy, ay, elemLen, inftyP);
   cpMaskedReplace_ct(pRz, mont1, elemLen, inftyP);
   cpMaskedReplace_ct(pRz, ax, elemLen, inftyP&inftyA);

   cpMaskedReplace_ct(pRx, px, elemLen*3, inftyA);

   cpGFpElementCopy(pRdata, pRx, 3*elemLen);
}
#endif

