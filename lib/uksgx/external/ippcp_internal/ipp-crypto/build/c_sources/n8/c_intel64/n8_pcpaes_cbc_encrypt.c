/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
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
//     AES encryption (CBC mode)
//     AES encryption (CBC-CS mode)
//
//  Contents:
//     cpEncryptAES_cbc()
//
*/

#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "pcpaes_cbc_encrypt.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

/*
// AES-CBC ecnryption
//
// Parameters:
//    pIV         pointer to the initialization vector
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    nBlocks     number of ecnrypted data blocks
//    pCtx        pointer to the AES context
*/
IPP_OWN_DEFN (void, cpEncryptAES_cbc, (const Ipp8u* pIV, const Ipp8u* pSrc, Ipp8u* pDst, int nBlocks, const IppsAESSpec* pCtx))
{
#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   if(AES_NI_ENABLED==RIJ_AESNI(pCtx)) {
      EncryptCBC_RIJ128_AES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), nBlocks*MBS_RIJ128, pIV);
   }
   else
#endif
   {
      /* setup encoder method */
      RijnCipher encoder = RIJ_ENCODER(pCtx);

      /* read IV */
      Ipp32u iv[NB(128)];
      CopyBlock16(pIV, iv);

      /* block-by-block encryption */
      while(nBlocks) {
         iv[0] ^= ((Ipp32u*)pSrc)[0];
         iv[1] ^= ((Ipp32u*)pSrc)[1];
         iv[2] ^= ((Ipp32u*)pSrc)[2];
         iv[3] ^= ((Ipp32u*)pSrc)[3];

         //encoder(iv, (Ipp32u*)pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), (const Ipp32u (*)[256])RIJ_ENC_SBOX(pCtx));
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder((Ipp8u*)iv, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), RijEncSbox/*NULL*/);
         #else
         encoder((Ipp8u*)iv, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), NULL);
         #endif

         iv[0] = ((Ipp32u*)pDst)[0];
         iv[1] = ((Ipp32u*)pDst)[1];
         iv[2] = ((Ipp32u*)pDst)[2];
         iv[3] = ((Ipp32u*)pDst)[3];

         pSrc += MBS_RIJ128;
         pDst += MBS_RIJ128;
         nBlocks--;
      }
   }
}
