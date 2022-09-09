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
//     Initialization of AES
// 
//  Contents:
//        ippsAESInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcprij128safe.h"
#include "pcptool.h"

/*F*
//    Name: ippsAESInit
//
// Purpose: Init AES context for future usage
//          and setup secret key.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//    ippStsMemAllocErr       size of buffer is not match for operation
//    ippStsLengthErr         keyLen != 16
//                            keyLen != 24
//                            keyLen != 32
//    ippStsNoErr             no errors
//
// Parameters:
//    pKey        secret key
//    keyLen      length of the secret key (in bytes)
//    pCtx        pointer to buffer initialized as AES context
//    ctxSize     available size (in bytes) of buffer above
//
// Note:
//    if pKey==NULL, then AES initialized by zero value key
//
*F*/
IPPFUN(IppStatus, ippsAESInit,(const Ipp8u* pKey, int keyLen,
                               IppsAESSpec* pCtx, int ctxSize))
{
   /* test context pointer */
   IPP_BAD_PTR1_RET(pCtx);

   /* make sure in legal keyLen */
   IPP_BADARG_RET(keyLen!=16 && keyLen!=24 && keyLen!=32, ippStsLengthErr);

   IPP_BADARG_RET(((Ipp8u*)pCtx+sizeof(IppsAESSpec)) > ((Ipp8u*)pCtx+ctxSize), ippStsMemAllocErr);

   {
      int keyWords = NK(keyLen*BITSIZE(Ipp8u));
      int nExpKeys = rij128nKeys  [ rij_index(keyWords) ];
      int nRounds  = rij128nRounds[ rij_index(keyWords) ];

      Ipp8u zeroKey[32] = {0};
      const Ipp8u* pActualKey = pKey? pKey : zeroKey;

      /* clear context */
      PadBlock(0, pCtx, sizeof(IppsAESSpec));

      /* init spec */
      /* light trick to prevent context copy: add low 32-bit part of its current address to the context Id and
       * check if is changed later in processing functions */
      RIJ_SET_ID(pCtx);
      RIJ_NB(pCtx) = NB(128);
      RIJ_NK(pCtx) = keyWords;
      RIJ_NR(pCtx) = nRounds;
      RIJ_SAFE_INIT(pCtx) = 1;

      /* align addresses of keys buffer */
      RIJ_EKEYS(pCtx) = (Ipp8u*)(IPP_ALIGNED_PTR(RIJ_KEYS_BUFFER(pCtx), AES_ALIGNMENT));
      RIJ_DKEYS(pCtx) = (Ipp8u*)((Ipp32u*)RIJ_EKEYS(pCtx) + nExpKeys);

      #if (_AES_NI_ENABLING_==_FEATURE_ON_)
         RIJ_AESNI(pCtx) = AES_NI_ENABLED;
         RIJ_ENCODER(pCtx) = Encrypt_RIJ128_AES_NI; /* AES_NI based encoder */
         RIJ_DECODER(pCtx) = Decrypt_RIJ128_AES_NI; /* AES_NI based decoder */
         cpExpandAesKey_NI(pActualKey, pCtx);       /* AES_NI based key expansion */

      #else
         #if (_AES_NI_ENABLING_==_FEATURE_TICKTOCK_)
         if( IsFeatureEnabled(ippCPUID_AES) ) {
            RIJ_AESNI(pCtx) = AES_NI_ENABLED;
            RIJ_ENCODER(pCtx) = Encrypt_RIJ128_AES_NI; /* AES_NI based encoder */
            RIJ_DECODER(pCtx) = Decrypt_RIJ128_AES_NI; /* AES_NI based decoder */
            cpExpandAesKey_NI(pActualKey, pCtx);       /* AES_NI based key expansion */
         }
         else
         #endif
         {
            ExpandRijndaelKey(pActualKey, keyWords, NB(128), nRounds, nExpKeys,
                           RIJ_EKEYS(pCtx),
                           RIJ_DKEYS(pCtx));

            #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPOSITE_GF_)
            {
               int nr;
               Ipp8u* pEnc_key = (Ipp8u*)(RIJ_EKEYS(pCtx));
               Ipp8u* pDec_key = (Ipp8u*)(RIJ_DKEYS(pCtx));
               /* update key material: convert into GF((2^4)2) */
               for(nr=0; nr<(1+nRounds); nr++) {
                TransformNative2Composite(pEnc_key+16*nr, pEnc_key+16*nr);
                TransformNative2Composite(pDec_key+16*nr, pDec_key+16*nr);
                }
               RIJ_ENCODER(pCtx) = SafeEncrypt_RIJ128; /* safe encoder (composite GF) */
               RIJ_DECODER(pCtx) = SafeDecrypt_RIJ128; /* safe decoder (composite GF)*/
            }
            #else
            {
               int nr;
               Ipp8u* pEnc_key = (Ipp8u*)(RIJ_EKEYS(pCtx));
               /* update key material: transpose inplace */
               for(nr=0; nr<(1+nRounds); nr++, pEnc_key+=16) {
                  SWAP(pEnc_key[ 1], pEnc_key[ 4]);
                  SWAP(pEnc_key[ 2], pEnc_key[ 8]);
                  SWAP(pEnc_key[ 3], pEnc_key[12]);
                  SWAP(pEnc_key[ 6], pEnc_key[ 9]);
                  SWAP(pEnc_key[ 7], pEnc_key[13]);
                  SWAP(pEnc_key[11], pEnc_key[14]);
               }
               RIJ_ENCODER(pCtx) = Safe2Encrypt_RIJ128; /* safe encoder (compact Sbox)) */
               RIJ_DECODER(pCtx) = Safe2Decrypt_RIJ128; /* safe decoder (compact Sbox)) */
            }
            #endif
         }
      #endif

      return ippStsNoErr;
   }
}
