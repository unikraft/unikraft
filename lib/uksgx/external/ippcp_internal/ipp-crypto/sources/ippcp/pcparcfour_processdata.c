/*******************************************************************************
* Copyright 2005-2021 Intel Corporation
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
//     RC4 implementation
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcparcfour.h"
#include "pcptool.h"


/*
// data processing function.
*/
#if !((_IPP>=_IPP_M5) || (_IPP32E>=_IPP32E_M7))

IPP_OWN_DEFN (void, ARCFourProcessData, (const Ipp8u *pSrc, Ipp8u *pDst, int length, IppsARCFourState *pCtx))
{
   if(length) {
      rc4word tx, ty;

      Ipp32u x = RC4_CNTX(pCtx);
      Ipp32u y = RC4_CNTY(pCtx);
      rc4word* pSbox = RC4_SBOX(pCtx);

      x = (x+1) &0xFF;
      tx = pSbox[x];

      while(length) {
         y = (y+tx) & 0xFF;
         ty = pSbox[y];
         pSbox[x] = ty;
         x = (x+1) & 0xFF;
         ty = (ty+tx) & 0xFF;
         pSbox[y] = tx;
         tx = pSbox[x];
         ty = pSbox[ty];
         *pDst = (Ipp8u)( *pSrc ^ ty );
         pDst++;
         pSrc++;
         length--;
      }
      RC4_CNTX(pCtx) = (x-1) & 0xFF;
      RC4_CNTY(pCtx) = y;
   }
}

#endif /* #if !((_IPP>=_IPP_M5) || (_IPP32E>=_IPP32E_M7)) */

