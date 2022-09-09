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
//     Cryptography Primitive.
//     Encrypt/Decrypt byte data stream according to Rijndael128 (GCM mode)
// 
//     "fast" stuff
// 
//  Contents:
//      wrpAesGcmDec_table2K()
// 
*/


#include "owndefs.h"
#include "owncp.h"

#include "pcpaesauthgcm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

#if(_IPP32E<_IPP32E_K0)

/*
// authenticates and decrypts n*BLOCK_SIZE bytes
*/
IPP_OWN_DEFN (void, wrpAesGcmDec_table2K, (Ipp8u* pDst, const Ipp8u* pSrc, int len, IppsAES_GCMState* pState))
{
   //AesGcmAuth_table2K(AESGCM_GHASH(pState), pSrc, len, AESGCM_HKEY(pState), AesGcmConst_table);
   AesGcmAuth_table2K_ct(AESGCM_GHASH(pState), pSrc, len, AESGCM_HKEY(pState), AesGcmConst_table);

   {
      Ipp8u* pCounter = AESGCM_COUNTER(pState);
      Ipp8u* pECounter = AESGCM_ECOUNTER(pState);

      IppsRijndael128Spec* pAES = AESGCM_CIPHER(pState);
      RijnCipher encoder = RIJ_ENCODER(pAES);

      while(len>=BLOCK_SIZE) {
         /* encrypt whole AES block */
         XorBlock16(pSrc, pECounter, pDst);

         pSrc += BLOCK_SIZE;
         pDst += BLOCK_SIZE;
         len -= BLOCK_SIZE;

         /* increment counter block */
         IncrementCounter32(pCounter);
         /* and encrypt counter */
         //encoder((Ipp32u*)pCounter, (Ipp32u*)pECounter, RIJ_NR(pAES), RIJ_EKEYS(pAES), (const Ipp32u (*)[256])RIJ_ENC_SBOX(pAES));
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder(pCounter, pECounter, RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
         #else
         encoder(pCounter, pECounter, RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
         #endif
      }
   }
}

#endif
