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
//        ippsRSASign_PKCS1v15()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpngrsa.h"
#include "pcphash.h"
#include "pcptool.h"

#include "pcprsa_pkcs1c15_data.h"
#include "pcprsa_generatesign_pkcs1v15.h"

IPPFUN(IppStatus, ippsRSASign_PKCS1v15,(const Ipp8u* pMsg, int msgLen,
                                              Ipp8u* pSign,
                                        const IppsRSAPrivateKeyState* pPrvKey,
                                        const IppsRSAPublicKeyState*  pPubKey,
                                              IppHashAlgId hashAlg,
                                              Ipp8u* pScratchBuffer))
{
   /* test private key context */
   IPP_BAD_PTR2_RET(pPrvKey, pScratchBuffer);
   IPP_BADARG_RET(!RSA_PRV_KEY_VALID_ID(pPrvKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PRV_KEY_IS_SET(pPrvKey), ippStsIncompleteContextErr);

   /* test hash algorith ID */
   hashAlg = cpValidHashAlg(hashAlg);
   IPP_BADARG_RET(ippHashAlg_Unknown==hashAlg, ippStsNotSupportedModeErr);
   IPP_BADARG_RET(ippHashAlg_SM3==hashAlg, ippStsNotSupportedModeErr);

   /* use aligned public key context if defined */
   if(pPubKey) {
      IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(pPubKey), ippStsContextMatchErr);
      IPP_BADARG_RET(!RSA_PUB_KEY_IS_SET(pPubKey), ippStsIncompleteContextErr);
   }

   /* test data pointer */
   IPP_BAD_PTR2_RET(pMsg, pSign);
   /* test length */
   IPP_BADARG_RET(msgLen<0, ippStsLengthErr);

   {
      Ipp8u md[IPP_SHA512_DIGEST_BITSIZE/BYTESIZE];
      int mdLen = cpHashSize(hashAlg);
      ippsHashMessage(pMsg, msgLen, md, hashAlg);

      {
         const Ipp8u* pSalt = pksc15_salt[hashAlg].pSalt;
         int saltLen = pksc15_salt[hashAlg].saltLen;

         int sts = GenerateSign(md, mdLen,
                         pSalt, saltLen,
                         pSign,
                         pPrvKey, pPubKey,
                         (BNU_CHUNK_T*)(IPP_ALIGNED_PTR((pScratchBuffer), (int)sizeof(BNU_CHUNK_T))));

         return (1==sts)? ippStsNoErr : ippStsSizeErr;
      }
   }
}
