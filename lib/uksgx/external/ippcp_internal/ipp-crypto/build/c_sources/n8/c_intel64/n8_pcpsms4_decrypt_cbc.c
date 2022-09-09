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
//        cpDecryptSMS4_cbc()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"
#include "pcpsms4_decrypt_cbc.h"

/*F*
//    Name: cpDecryptSMS4_cbc
//
// Purpose: SMS4-CBC decryption.
//
// Parameters:
//    pIV         pointer to the initialization vector
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    dataLen     input/output buffer length (in bytes)
//    pCtx        pointer to the SMS4 context
//
*F*/
IPP_OWN_DEFN (void, cpDecryptSMS4_cbc, (const Ipp8u* pIV, const Ipp8u* pSrc, Ipp8u* pDst, int dataLen, const IppsSMS4Spec* pCtx))
{
   const Ipp32u* pRoundKeys = SMS4_DRK(pCtx);

   __ALIGN16 Ipp32u TMP[2*(MBS_SMS4/sizeof(Ipp32u))];

   /*
      iv           size = MBS_SMS4/sizeof(Ipp32u)
      tmp          size = MBS_SMS4/sizeof(Ipp32u)
   */

   Ipp32u*          iv = TMP;
   Ipp32u*          tmp = TMP + (MBS_SMS4/sizeof(Ipp32u));

   /* copy IV */
   CopyBlock16(pIV, iv);

   /* do decryption */
   #if (_IPP32E>=_IPP32E_K1)
   #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)
   if (IsFeatureEnabled(ippCPUID_AVX512GFNI)) {
      int processedLen = cpSMS4_CBC_dec_gfni512(pDst, pSrc, dataLen, pRoundKeys, (Ipp8u*)iv);
      pSrc += processedLen;
      pDst += processedLen;
      dataLen -= processedLen;
   }
   else
   #endif /* #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */
   #endif /* (_IPP32E>=_IPP32E_K1) */
   #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   if(IsFeatureEnabled(ippCPUID_AES)) {
      int processedLen = cpSMS4_CBC_dec_aesni(pDst, pSrc, dataLen, pRoundKeys, (Ipp8u*)iv);
      pSrc += processedLen;
      pDst += processedLen;
      dataLen -= processedLen;
   }
   #endif

   for(; dataLen>0; dataLen-=MBS_SMS4, pSrc+=MBS_SMS4, pDst+=MBS_SMS4) {

      cpSMS4_Cipher((Ipp8u*)tmp, (Ipp8u*)pSrc, pRoundKeys);

      tmp[0] ^= iv[0];
      tmp[1] ^= iv[1];
      tmp[2] ^= iv[2];
      tmp[3] ^= iv[3];

      iv[0] = ((Ipp32u*)pSrc)[0];
      iv[1] = ((Ipp32u*)pSrc)[1];
      iv[2] = ((Ipp32u*)pSrc)[2];
      iv[3] = ((Ipp32u*)pSrc)[3];

      CopyBlock16(tmp, pDst);
   }

   /* clear secret data */
   PurgeBlock(TMP, sizeof(TMP));
}
