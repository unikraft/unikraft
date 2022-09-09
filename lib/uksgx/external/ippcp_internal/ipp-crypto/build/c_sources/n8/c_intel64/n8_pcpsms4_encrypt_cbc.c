/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     SMS4 encryption/decryption
// 
//  Contents:
//        cpEncryptSMS4_cbc()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"
#include "pcpsms4_encrypt_cbc.h"

/*F*
//
//    Name: cpEncryptSMS4_cbc
//
// Purpose: SMS4-CBC encryption.
//
// Parameters:
//    pIV         pointer to the initialization vector
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    dataLen     input/output buffer length (in bytes)
//    pCtx        pointer to the SMS4 context
//
*F*/
IPP_OWN_DEFN (void, cpEncryptSMS4_cbc, (const Ipp8u* pIV, const Ipp8u* pSrc, Ipp8u* pDst, int dataLen, const IppsSMS4Spec* pCtx))
{
   const Ipp32u* pRoundKeys = SMS4_RK(pCtx);

   /* read IV */
   __ALIGN16 Ipp32u iv[MBS_SMS4/sizeof(Ipp32u)];
   CopyBlock16(pIV, iv);

   /* do encryption */
   for(; dataLen>0; dataLen-=MBS_SMS4, pSrc+=MBS_SMS4, pDst+=MBS_SMS4) {
      iv[0] ^= ((Ipp32u*)pSrc)[0];
      iv[1] ^= ((Ipp32u*)pSrc)[1];
      iv[2] ^= ((Ipp32u*)pSrc)[2];
      iv[3] ^= ((Ipp32u*)pSrc)[3];

      cpSMS4_Cipher(pDst, (Ipp8u*)iv, pRoundKeys);

      iv[0] = ((Ipp32u*)pDst)[0];
      iv[1] = ((Ipp32u*)pDst)[1];
      iv[2] = ((Ipp32u*)pDst)[2];
      iv[3] = ((Ipp32u*)pDst)[3];
   }

   /* clear secret data */
   PurgeBlock(iv, sizeof(iv));
}
