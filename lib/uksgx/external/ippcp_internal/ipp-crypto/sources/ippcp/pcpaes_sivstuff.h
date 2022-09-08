/*******************************************************************************
* Copyright 2015-2021 Intel Corporation
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
//     AES-SIV Functions (RFC 5297)
// 
//  Contents:
//        Stuff()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpcmac.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if !defined(_PCP_AES_SIV_STUFF_H_)

#define _PCP_AES_SIV_STUFF_H_
////////////////////////////////////////////////////////////
__INLINE void cpAES_CMAC(Ipp8u mac[MBS_RIJ128], const Ipp8u* pSrc, int len, IppsAES_CMACState* pCtx)
{
   ippsAES_CMACUpdate(pSrc, len, pCtx);
   ippsAES_CMACFinal(mac, MBS_RIJ128, pCtx);
}
__INLINE IppStatus cpAES_S2V_init(Ipp8u v[MBS_RIJ128], const Ipp8u* pKey, int keyLen, IppsAES_CMACState* pCtx, int ctxSize)
{
   IppStatus sts = ippsAES_CMACInit(pKey, keyLen, pCtx, ctxSize);
   if(ippStsNoErr==sts) {
      PadBlock(0, v, MBS_RIJ128);
      cpAES_CMAC(v, v, MBS_RIJ128, pCtx);
   }
   return sts;
}
__INLINE Ipp8u* double16(Ipp8u out[MBS_RIJ128], const Ipp8u inp[MBS_RIJ128])
{
   /* double inp */
   Ipp32u carry = 0;
   int n;
   for(n=0; n<MBS_RIJ128; n++) {
      Ipp32u x = inp[MBS_RIJ128-1-n] + inp[MBS_RIJ128-1-n] + carry;
      out[MBS_RIJ128-1-n] = (Ipp8u)x;
      carry = (x>>8) & 0xFF;
   }

   out[MBS_RIJ128-1] ^= ((Ipp8u)(0-carry) & 0x87);
   return out;
}
__INLINE void cpAES_S2V_update(Ipp8u v[MBS_RIJ128], const Ipp8u* pSrc, int len, IppsAES_CMACState* pCtx)
{
   Ipp8u t[MBS_RIJ128];
   cpAES_CMAC(t, pSrc, len, pCtx);
   double16(v, v);
   XorBlock16(v, t, v);
}

static void cpAES_S2V_final(Ipp8u v[MBS_RIJ128], const Ipp8u* pSrc, int len, IppsAES_CMACState* pCtx)
{
   Ipp8u t[MBS_RIJ128];

   if(len>=MBS_RIJ128) {
      ippsAES_CMACUpdate(pSrc, len-MBS_RIJ128, pCtx);
      XorBlock16(pSrc+len-MBS_RIJ128, v, t);
   }
   else {
      double16(t, v);
      XorBlock(pSrc, t, t, len);
      t[len] ^= 0x80;
   }
   cpAES_CMAC(v, t, MBS_RIJ128, pCtx);
}

#endif /*_PCP_AES_SIV_STUFF_H_*/
