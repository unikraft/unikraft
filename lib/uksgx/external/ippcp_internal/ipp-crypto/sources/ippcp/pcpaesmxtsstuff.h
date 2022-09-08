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
// 
//  Purpose:
//     Cryptography Primitive.
//     AES-XTS Internal Functions
// 
// 
*/

#if !defined(_PCP_AES_XTS_STUFF_H)
#define _PCP_AES_XTS_STUFF_H

#include "owncp.h"
#include "pcpaesm.h"

/*
   multiplication by primirive element alpha (==2)
   over P=x^128 +x^7 +x^2 +x +1

   LE version
*/

#define GF_MASK   (0x8000000000000000)
#define GF_POLY   (0x0000000000000087)

__INLINE void gf_mul_by_primitive(void* x)
{
   Ipp64u* x64 = (Ipp64u*)x;
   Ipp64u xorL = ((Ipp64s)x64[1] >> 63) & GF_POLY;
   Ipp64u addH = ((Ipp64s)x64[0] >> 63) & 1;
   x64[0] = (x64[0]+x64[0]) ^ xorL;
   x64[1] = (x64[1]+x64[1]) + addH;
}

/*
   the following are especially for multi-block processing
*/
static void cpXTSwhitening(Ipp8u* buffer, int nblk, Ipp8u* ptwk)
{
   Ipp64u* pbuf64 = (Ipp64u*)buffer;
   Ipp64u* ptwk64 = (Ipp64u*)ptwk;

   pbuf64[0] = ptwk64[0];
   pbuf64[1] = ptwk64[1];

   for(nblk--, pbuf64+=2; nblk>0; nblk--, pbuf64+=2) {
      gf_mul_by_primitive(ptwk64);
      pbuf64[0] = ptwk64[0];
      pbuf64[1] = ptwk64[1];
   }
   gf_mul_by_primitive(ptwk64);
}

static void cpXTSxor16(Ipp8u* pDst, const Ipp8u* pSrc1, const Ipp8u* pSrc2, int nblk)
{
   Ipp64u* pdst64 = (Ipp64u*)pDst;
   const Ipp64u* ps1_64 = (const Ipp64u*)pSrc1;
   const Ipp64u* ps2_64 = (const Ipp64u*)pSrc2;
   for(; nblk>0; nblk--, pdst64+=2, ps1_64+=2, ps2_64+=2) {
      pdst64[0] = ps1_64[0] ^ ps2_64[0];
      pdst64[1] = ps1_64[1] ^ ps2_64[1];
   }
}

#endif /* _PCP_AES_XTS_STUFF_H */
