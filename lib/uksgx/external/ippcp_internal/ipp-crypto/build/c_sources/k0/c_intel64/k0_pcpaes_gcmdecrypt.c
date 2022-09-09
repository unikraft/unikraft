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
//        ippsAES_GCMDecrypt()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

#if(_IPP32E>=_IPP32E_K0)
#include "pcpaesauthgcm_avx512.h"
#else
#include "pcpaesauthgcm.h"
#endif /* #if(_IPP32E>=_IPP32E_K0) */

/*F*
//    Name: ippsAES_GCMDecrypt
//
// Purpose: Decrypts a data buffer in the GCM mode.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSrc == NULL
//                            pDst == NULL
//                            pState == NULL
//    ippStsContextMatchErr  !AESGCM_VALID_ID()
//    ippStsLengthErr         len<0
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        Pointer to ciphertext.
//    pDst        Pointer to plaintext.
//    len         Length of the plaintext and ciphertext in bytes
//    pState      pointer to the context
//
*F*/

IPPFUN(IppStatus, ippsAES_GCMDecrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len, IppsAES_GCMState* pState))
{
   /* test pState pointer */
   IPP_BAD_PTR1_RET(pState);
   /* use aligned context */
   pState = (IppsAES_GCMState*)( IPP_ALIGNED_PTR(pState, AESGCM_ALIGNMENT) );
   /* test state ID */
   IPP_BADARG_RET(!AESGCM_VALID_ID(pState), ippStsContextMatchErr);
   /* test context validity */
   IPP_BADARG_RET(!(GcmAADprocessing==AESGCM_STATE(pState) || GcmTXTprocessing==AESGCM_STATE(pState)), ippStsBadArgErr);

   /* test text pointers and length */
   IPP_BAD_PTR2_RET(pSrc, pDst);
   IPP_BADARG_RET(len<0, ippStsLengthErr);

   #if(_IPP32E<_IPP32E_K0)

   /* get method */
   IppsAESSpec* pAES = AESGCM_CIPHER(pState);
   RijnCipher encoder = RIJ_ENCODER(pAES);
   MulGcm_ hashFunc = AESGCM_HASH(pState);
   
   #endif

   if( GcmAADprocessing==AESGCM_STATE(pState) ) {

      #if(_IPP32E>=_IPP32E_K0)

      AadFinalize_ aadHashFinalize = AES_GCM_AAD_FINALIZE(pState);

      aadHashFinalize(&AES_GCM_KEY_DATA(pState), &AES_GCM_CONTEXT_DATA(pState), 
                      AESGCM_GHASH(pState), (Ipp64u)AESGCM_BUFLEN(pState), AESGCM_AAD_LEN(pState));

      #else
      
      /* complete AAD processing */
      if(AESGCM_BUFLEN(pState))
         hashFunc(AESGCM_GHASH(pState), AESGCM_HKEY(pState), AesGcmConst_table);

      /* increment counter block */
      IncrementCounter32(AESGCM_COUNTER(pState));
      /* and encrypt counter */
      #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
      encoder(AESGCM_COUNTER(pState), AESGCM_ECOUNTER(pState), RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
      #else
      encoder(AESGCM_COUNTER(pState), AESGCM_ECOUNTER(pState), RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
      #endif

      #endif /* #if(_IPP32E>=_IPP32E_K0) */

      /* switch mode and init counters */
      AESGCM_BUFLEN(pState) = 0; 
      AESGCM_TXT_LEN(pState) = CONST_64(0);
      AESGCM_STATE(pState) = GcmTXTprocessing;
   }

   /*
   // process text (authenticate and decrypt)
   */

   #if(_IPP32E>=_IPP32E_K0)

   DecryptUpdate_ dec = AES_GCM_DECRYPT_UPDATE(pState);
   dec(&AES_GCM_KEY_DATA(pState), &AES_GCM_CONTEXT_DATA(pState), pDst, pSrc, (Ipp64u)len);

   #else

   /* process partial block */
   if(AESGCM_BUFLEN(pState)) {
      int locLen = IPP_MIN(len, BLOCK_SIZE-AESGCM_BUFLEN(pState));
      /* authentication */
      XorBlock(pSrc, AESGCM_GHASH(pState)+AESGCM_BUFLEN(pState), AESGCM_GHASH(pState)+AESGCM_BUFLEN(pState), locLen);
      /* ctr decryption */
      XorBlock(pSrc, AESGCM_ECOUNTER(pState)+AESGCM_BUFLEN(pState), pDst, locLen);

      AESGCM_BUFLEN(pState) += locLen;
      AESGCM_TXT_LEN(pState) += (Ipp64u)locLen;
      pSrc += locLen;
      pDst += locLen;
      len -= locLen;

      /* if buffer full */
      if(BLOCK_SIZE==AESGCM_BUFLEN(pState)) {
         /* hash buffer */
         hashFunc(AESGCM_GHASH(pState), AESGCM_HKEY(pState), AesGcmConst_table);
         AESGCM_BUFLEN(pState) = 0;

         /* increment counter block */
         IncrementCounter32(AESGCM_COUNTER(pState));
         /* and encrypt counter */
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder(AESGCM_COUNTER(pState), AESGCM_ECOUNTER(pState), RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
         #else
         encoder(AESGCM_COUNTER(pState), AESGCM_ECOUNTER(pState), RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
         #endif
      }
   }

   /* process the main part of text */
   {
      int lenBlks = len & (-BLOCK_SIZE);
      if(lenBlks) {
         Decrypt_ decFunc = AESGCM_DEC(pState);

         decFunc(pDst, pSrc, lenBlks, pState);

         AESGCM_TXT_LEN(pState) += (Ipp64u)lenBlks;
         pSrc += lenBlks;
         pDst += lenBlks;
         len -= lenBlks;
      }
   }

   /* process the rest of text */
   if(len) {
      /* ctr encryption */
      XorBlock(pSrc, AESGCM_GHASH(pState)+AESGCM_BUFLEN(pState), AESGCM_GHASH(pState)+AESGCM_BUFLEN(pState), len);
      XorBlock(pSrc, AESGCM_ECOUNTER(pState)+AESGCM_BUFLEN(pState), pDst, len);

      AESGCM_BUFLEN(pState) += len;
      AESGCM_TXT_LEN(pState) += (Ipp64u)len;
   }

   #endif /* #if(_IPP32E>=_IPP32E_K0) */

   return ippStsNoErr;
}
