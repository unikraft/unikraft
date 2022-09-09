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
//        ippsRSAVerify_PKCS1v15()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpngrsa.h"
#include "pcphash.h"
#include "pcptool.h"

#include "pcprsa_pkcs1c15_data.h"
#include "pcprsa_verifysign_pkcs1v15.h"

IPPFUN(IppStatus, ippsRSAVerify_PKCS1v15,(const Ipp8u* pMsg, int msgLen,
                                          const Ipp8u* pSign, int* pIsValid,
                                          const IppsRSAPublicKeyState* pKey,
                                                IppHashAlgId hashAlg,
                                                Ipp8u* pScratchBuffer))
{
   /* test public key context */
   IPP_BAD_PTR2_RET(pKey, pScratchBuffer);
   IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(pKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PUB_KEY_IS_SET(pKey), ippStsIncompleteContextErr);

   /* test hash algorith ID */
   hashAlg = cpValidHashAlg(hashAlg);
   IPP_BADARG_RET(ippHashAlg_Unknown==hashAlg, ippStsNotSupportedModeErr);
   IPP_BADARG_RET(ippHashAlg_SM3==hashAlg, ippStsNotSupportedModeErr);

   /* test data pointer */
   IPP_BAD_PTR3_RET(pMsg, pSign, pIsValid);
   /* test length */
   IPP_BADARG_RET(msgLen<0, ippStsLengthErr);

   *pIsValid = 0;
   {
      Ipp8u md[IPP_SHA512_DIGEST_BITSIZE/BYTESIZE];
      int mdLen = cpHashSize(hashAlg);
      ippsHashMessage(pMsg, msgLen, md, hashAlg);

      return VerifySign(md, mdLen,
                        pksc15_salt[hashAlg].pSalt, pksc15_salt[hashAlg].saltLen,
                        pSign, pIsValid,
                        pKey,
                        (BNU_CHUNK_T*)(IPP_ALIGNED_PTR((pScratchBuffer), (int)sizeof(BNU_CHUNK_T))))? ippStsNoErr : ippStsSizeErr;
   }
}
