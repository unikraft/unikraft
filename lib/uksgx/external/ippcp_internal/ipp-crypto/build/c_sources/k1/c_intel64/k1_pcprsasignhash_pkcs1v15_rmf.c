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
//     RSASSA-PKCS-v1_5
// 
//     Signatire Scheme with Appendix Signatute Generation
// 
//  Contents:
//        ippsRSASignHash_PKCS1v15_rmf()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpngrsa.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

#include "pcprsa_pkcs1c15_data.h"
#include "pcprsa_generatesign_pkcs1v15.h"

#if defined( _ABL_ )

IPPFUN(IppStatus, ippsRSASignHash_PKCS1v15_rmf,(const Ipp8u* md,
                                                      Ipp8u* pSign,
                                                const IppsRSAPrivateKeyState* pPrvKey,
                                                const IppsRSAPublicKeyState*  pPubKey,
                                                const IppsHashMethod* pMethod,
                                                      Ipp8u* pScratchBuffer))
{
   IppHashAlgId hashAlg;

   /* test private key context */
   IPP_BAD_PTR3_RET(pPrvKey, pScratchBuffer, pMethod);
   IPP_BADARG_RET(!RSA_PRV_KEY_VALID_ID(pPrvKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PRV_KEY_IS_SET(pPrvKey), ippStsIncompleteContextErr);

   /* test hash algorith ID */
   hashAlg = pMethod->hashAlgId;
   IPP_BADARG_RET(ippHashAlg_SM3==hashAlg, ippStsNotSupportedModeErr);

   /* use aligned public key context if defined */
   if(pPubKey) {
      IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(pPubKey), ippStsContextMatchErr);
      IPP_BADARG_RET(!RSA_PUB_KEY_IS_SET(pPubKey), ippStsIncompleteContextErr);
   }

   /* test data pointer */
   IPP_BAD_PTR2_RET(md, pSign);

   {
      const Ipp8u* pSalt = pksc15_salt[hashAlg].pSalt;
      int saltLen = pksc15_salt[hashAlg].saltLen;

      int sts = GenerateSign(md, pMethod->hashLen,
                         pSalt, saltLen,
                         pSign,
                         pPrvKey, pPubKey,
                         (BNU_CHUNK_T*)(IPP_ALIGNED_PTR((pScratchBuffer), (int)sizeof(BNU_CHUNK_T))));

      return (1==sts)? ippStsNoErr : ippStsSizeErr;
   }
}

#endif /* #if defined( _ABL_ ) */

