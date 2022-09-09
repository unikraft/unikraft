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
//     AES-GCM
// 
//  Contents:
//        ippsAES_GCMProcessIV()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if(_IPP32E>=_IPP32E_K0)
#include "pcpaesauthgcm_avx512.h"
#else
#include "pcpaesauthgcm.h"
#endif /* #if(_IPP32E>=_IPP32E_K0) */

/*F*
//    Name: ippsAES_GCMProcessIV
//
// Purpose: IV processing.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pState == NULL
//                            pIV ==NULL && ivLen>0
//    ippStsContextMatchErr   !AESGCM_VALID_ID()
//    ippStsLengthErr         ivLen <0
//    ippStsBadArgErr         illegal sequence call
//    ippStsNoErr             no errors
//
// Parameters:
//    pIV         pointer to the IV
//    ivLen       length of IV (it could be 0)
//    pState      pointer to the context
//
*F*/
IPPFUN(IppStatus, ippsAES_GCMProcessIV,(const Ipp8u* pIV, int ivLen, IppsAES_GCMState* pState))
{
   /* test pState pointer */
   IPP_BAD_PTR1_RET(pState);

   /* test IV pointer and length */
   IPP_BADARG_RET(ivLen && !pIV, ippStsNullPtrErr);
   IPP_BADARG_RET(ivLen<0, ippStsLengthErr);

   /* use aligned context */
   pState = (IppsAES_GCMState*)( IPP_ALIGNED_PTR(pState, AESGCM_ALIGNMENT) );
   /* test context validity */
   IPP_BADARG_RET(!AESGCM_VALID_ID(pState), ippStsContextMatchErr);

   IPP_BADARG_RET(!(GcmInit==AESGCM_STATE(pState) || GcmIVprocessing==AESGCM_STATE(pState)), ippStsBadArgErr);

   /* switch IVprocessing on */
   AESGCM_STATE(pState) = GcmIVprocessing;

   #if(_IPP32E>=_IPP32E_K0)

   IvUpdate_ ivHashUpdate = AES_GCM_IV_UPDATE(pState);

   /* test if buffer is not empty */
   if(AESGCM_BUFLEN(pState)) {
      int locLen = IPP_MIN(ivLen, BLOCK_SIZE-AESGCM_BUFLEN(pState));
      CopyBlock((void*)pIV, (void*)(AESGCM_COUNTER(pState)+AESGCM_BUFLEN(pState)), locLen);
      AESGCM_BUFLEN(pState) += locLen;

      /* if buffer full */
      if(BLOCK_SIZE==AESGCM_BUFLEN(pState)) {

         ivHashUpdate(&AES_GCM_KEY_DATA(pState), &AES_GCM_CONTEXT_DATA(pState), AESGCM_COUNTER(pState), BLOCK_SIZE);
         
         AESGCM_BUFLEN(pState) = 0;
      }

      AESGCM_IV_LEN(pState) += (Ipp64u)locLen;
      pIV += locLen;
      ivLen -= locLen;

   }

   /* process main part of IV */
   int lenBlks = ivLen & (-BLOCK_SIZE);
   if(lenBlks) {

      ivHashUpdate(&AES_GCM_KEY_DATA(pState), &AES_GCM_CONTEXT_DATA(pState), pIV, (Ipp64u)lenBlks);

      AESGCM_IV_LEN(pState) += (Ipp64u)lenBlks;
      pIV += lenBlks;
      ivLen -= lenBlks;
   }

   /* copy the rest of IV into the buffer */
   if(ivLen) {
      CopyBlock((void*)pIV, (void*)(AESGCM_COUNTER(pState)), ivLen);
      AESGCM_IV_LEN(pState) += (Ipp64u)ivLen;
      AESGCM_BUFLEN(pState) = ivLen;
   }

   #else

   /* test if buffer is not empty */
   if(AESGCM_BUFLEN(pState)) {
      int locLen = IPP_MIN(ivLen, BLOCK_SIZE-AESGCM_BUFLEN(pState));
      XorBlock(pIV, AESGCM_COUNTER(pState)+AESGCM_BUFLEN(pState), AESGCM_COUNTER(pState)+AESGCM_BUFLEN(pState), locLen);
      AESGCM_BUFLEN(pState) += locLen;

      /* if buffer full */
      if(BLOCK_SIZE==AESGCM_BUFLEN(pState)) {
         MulGcm_ ghashFunc = AESGCM_HASH(pState);
         ghashFunc(AESGCM_COUNTER(pState), AESGCM_HKEY(pState), AesGcmConst_table);
         AESGCM_BUFLEN(pState) = 0;
      }

      AESGCM_IV_LEN(pState) += (Ipp64u)locLen;
      pIV += locLen;
      ivLen -= locLen;
   }

   /* process main part of IV */
   {
      int lenBlks = ivLen & (-BLOCK_SIZE);
      if(lenBlks) {
         Auth_ authFunc = AESGCM_AUTH(pState);

         authFunc(AESGCM_COUNTER(pState), pIV, lenBlks, AESGCM_HKEY(pState), AesGcmConst_table);

         AESGCM_IV_LEN(pState) += (Ipp64u)lenBlks;
         pIV += lenBlks;
         ivLen -= lenBlks;
      }
   }

   /* copy the rest of IV into the buffer */
   if(ivLen) {
      XorBlock(pIV, AESGCM_COUNTER(pState), AESGCM_COUNTER(pState), ivLen);
      AESGCM_IV_LEN(pState) += (Ipp64u)ivLen;
      AESGCM_BUFLEN(pState) += ivLen;
   }

   #endif /* #if(_IPP32E>=_IPP32E_K0) */

   return ippStsNoErr;
}
