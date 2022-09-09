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

/*F*
//    Name: ippsARCFourInit
//
// Purpose: Init ARCFOUR spec for future usage.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//    ippStsLengthErr         1 > keyLen
//                            keyLen > IPP_ARCFOUR_KEYMAX_SIZE
//    ippStsNoErr             no errors
//
// Parameters:
//    key         security key
//    keyLen      length of key (bytes)
//    pCtx        pointer to the ARCFOUR context
//
*F*/
IPPFUN(IppStatus, ippsARCFourInit, (const Ipp8u *pKey, int keyLen, IppsARCFourState *pCtx))
{
   /* test context pointer */
   IPP_BAD_PTR1_RET(pCtx);

   /* test key */
   IPP_BAD_PTR1_RET(pKey);
   IPP_BADARG_RET(((1>keyLen)||(IPP_ARCFOUR_KEYMAX_SIZE< keyLen)), ippStsLengthErr);

   {
      int i;
      Ipp8u kblk[256], j, tmp;

      /* init RC4 context */
      RC4_SET_ID(pCtx);

      for(i=0; i<256; i++) {
         pCtx->Sbox0[i] = (Ipp8u)i;
         kblk[i] = pKey[i%keyLen];
      }
      j=0;
      for(i=0; i<256; i++) {
         j += pCtx->Sbox0[i] + kblk[i];
         tmp = pCtx->Sbox0[j];
         pCtx->Sbox0[j] = pCtx->Sbox0[i];
         pCtx->Sbox0[i] = tmp;
      }

      return ippsARCFourReset(pCtx);
   }
}
