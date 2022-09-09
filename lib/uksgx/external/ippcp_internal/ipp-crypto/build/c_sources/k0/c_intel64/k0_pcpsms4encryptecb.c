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
//        ippsSMS4EncryptECB()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

/*F*
//    Name: ippsSMS4EncryptECB
//
// Purpose: SMS4-ECB encryption.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//    ippStsContextMatchErr   !VALID_SMS4_ID()
//    ippStsLengthErr         len <1
//    ippStsUnderRunErr       0!=(len%MBS_SMS4)
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    len         input/output buffer length (in bytes)
//    pCtx        pointer to the SMS4 context
//
*F*/
IPPFUN(IppStatus, ippsSMS4EncryptECB,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_SMS4_ID(pCtx), ippStsContextMatchErr);

   /* test source and target buffer pointers */
   IPP_BAD_PTR2_RET(pSrc, pDst);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);
   /* test stream integrity */
   IPP_BADARG_RET((len&(MBS_SMS4-1)), ippStsUnderRunErr);

   /* do encryption */
   #if (_IPP32E>=_IPP32E_K1)
   #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)
   if (IsFeatureEnabled(ippCPUID_AVX512GFNI)) {
      int processedLen = cpSMS4_ECB_gfni512(pDst, pSrc, len, SMS4_ERK(pCtx));
      pSrc += processedLen;
      pDst += processedLen;
      len -= processedLen;
   }
   else
   #endif /* #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */
   #endif /* (_IPP32E>=_IPP32E_K1) */
   #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   if(IsFeatureEnabled(ippCPUID_AES)) {
      int processedLen = cpSMS4_ECB_aesni(pDst, pSrc, len, SMS4_ERK(pCtx));
      pSrc += processedLen;
      pDst += processedLen;
      len -= processedLen;
   }
   else
   #endif

   for(; len>0; len-=MBS_SMS4, pSrc+=MBS_SMS4, pDst+=MBS_SMS4)
      cpSMS4_Cipher(pDst, pSrc, SMS4_RK(pCtx));

   return ippStsNoErr;
}
