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
//     AES decryption (CBC mode)
//     AES decryption (CBC-CS mode)
//
//  Contents:
//     cpDecryptAES_cbc()
//
*/

#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "pcpaes_cbc_decrypt.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

/*
// AES-CBC decryption
//
// Parameters:
//    pIV         pointer to the initialization vector
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    nBlocks     number of decrypted data blocks
//    pCtx        pointer to the AES context
*/
IPP_OWN_DEFN (void, cpDecryptAES_cbc, (const Ipp8u* pIV, const Ipp8u* pSrc, Ipp8u* pDst, int nBlocks, const IppsAESSpec* pCtx))
{
#if(_IPP32E>=_IPP32E_K1)
   if (IsFeatureEnabled(ippCPUID_AVX512VAES)) {
      DecryptCBC_RIJ128pipe_VAES_NI(pSrc, pDst, nBlocks*MBS_RIJ128, pCtx, pIV);
   }
   else
#endif
#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   /* use pipelined version is possible */
   if(AES_NI_ENABLED==RIJ_AESNI(pCtx)) {
      DecryptCBC_RIJ128pipe_AES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_DKEYS(pCtx), nBlocks*MBS_RIJ128, pIV);
   }
   else
#endif
   {
      /* setup decoder method */
      RijnCipher decoder = RIJ_DECODER(pCtx);

      Ipp32u iv[NB(128)];

      /* copy IV */
      CopyBlock16(pIV, iv);

      /* not inplace block-by-block decryption */
      if(pSrc != pDst) {
         while(nBlocks) {
            //decoder((const Ipp32u*)pSrc, (Ipp32u*)pDst, RIJ_NR(pCtx), RIJ_DKEYS(pCtx), (const Ipp32u (*)[256])RIJ_DEC_SBOX(pCtx));
            #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
            decoder(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), RijDecSbox/*NULL*/);
            #else
            decoder(pSrc, pDst, RIJ_NR(pCtx), RIJ_DKEYS(pCtx), NULL);
            #endif

            ((Ipp32u*)pDst)[0] ^= iv[0];
            ((Ipp32u*)pDst)[1] ^= iv[1];
            ((Ipp32u*)pDst)[2] ^= iv[2];
            ((Ipp32u*)pDst)[3] ^= iv[3];

            iv[0] = ((Ipp32u*)pSrc)[0];
            iv[1] = ((Ipp32u*)pSrc)[1];
            iv[2] = ((Ipp32u*)pSrc)[2];
            iv[3] = ((Ipp32u*)pSrc)[3];

            pSrc += MBS_RIJ128;
            pDst += MBS_RIJ128;
            nBlocks--;
         }
      }
      /* inplace block-by-block decryption */
      else {
         Ipp32u tmpOut[NB(128)];

         while(nBlocks) {
            //decoder(pSrc, tmpOut, RIJ_NR(pCtx), RIJ_DKEYS(pCtx), (const Ipp32u (*)[256])RIJ_DEC_SBOX(pCtx));
            #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
            decoder(pSrc, (Ipp8u*)tmpOut, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), RijDecSbox/*NULL*/);
            #else
            decoder(pSrc, (Ipp8u*)tmpOut, RIJ_NR(pCtx), RIJ_DKEYS(pCtx), NULL);
            #endif

            tmpOut[0] ^= iv[0];
            tmpOut[1] ^= iv[1];
            tmpOut[2] ^= iv[2];
            tmpOut[3] ^= iv[3];

            iv[0] = ((Ipp32u*)pSrc)[0];
            iv[1] = ((Ipp32u*)pSrc)[1];
            iv[2] = ((Ipp32u*)pSrc)[2];
            iv[3] = ((Ipp32u*)pSrc)[3];

            CopyBlock16(tmpOut, pDst);

            pSrc += MBS_RIJ128;
            pDst += MBS_RIJ128;
            nBlocks--;
         }

         /* clear secret data */
         PurgeBlock(tmpOut, sizeof(tmpOut));
      }
   }
}
