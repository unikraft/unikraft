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
//        wrpAesGcmEnc_avx()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesauthgcm.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif


#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8) 
#if(_IPP32E<_IPP32E_K0)

/* encrypts and authenticates n*BLOCK_SIZE bytes */
IPP_OWN_DEFN (void, wrpAesGcmEnc_avx, (Ipp8u* pDst, const Ipp8u* pSrc, int lenBlks, IppsAES_GCMState* pState))
{
   IppsAESSpec* pAES = AESGCM_CIPHER(pState);
   RijnCipher encoder = RIJ_ENCODER(pAES);

   AesGcmEnc_avx(pDst, pSrc, lenBlks,
                 encoder, RIJ_NR(pAES), RIJ_EKEYS(pAES),
                 AESGCM_GHASH(pState),
                 AESGCM_COUNTER(pState),
                 AESGCM_ECOUNTER(pState),
                 AESGCM_HKEY(pState));
}

#endif /* (_IPP32E<_IPP32E_K0) */
#endif /* #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8) */

