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
//        ippsRSAVerify_PKCS1v15_rmf()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpngrsa.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

#include "pcprsa_pkcs1c15_data.h"
#include "pcprsa_verifysign_pkcs1v15.h"
#include "pcprsa_pkcs1v15_preproc.h"

IPPFUN(IppStatus, ippsRSAVerify_PKCS1v15_rmf,(const Ipp8u* pMsg, int msgLen,
                                              const Ipp8u* pSign, int* pIsValid,
                                              const IppsRSAPublicKeyState* pKey,
                                              const IppsHashMethod* pMethod,
                                                    Ipp8u* pScratchBuffer))
{
   const IppStatus preprocResult = SingleVerifyPkcs1v15RmfPreproc(pMsg, msgLen, pSign,
      pIsValid, &pKey, pMethod, pScratchBuffer); // badargs, pointer alignments, set valid = 0

   if (ippStsNoErr != preprocResult) {
      return preprocResult;
   }

   {
      Ipp8u md[IPP_SHA512_DIGEST_BITSIZE/BYTESIZE];
      ippsHashMessage_rmf(pMsg, msgLen, md, pMethod);

      return VerifySign(md, pMethod->hashLen,
                        pksc15_salt[pMethod->hashAlgId].pSalt, pksc15_salt[pMethod->hashAlgId].saltLen,
                        pSign, pIsValid,
                        pKey,
                        (BNU_CHUNK_T*)(IPP_ALIGNED_PTR((pScratchBuffer), (int)sizeof(BNU_CHUNK_T))))? ippStsNoErr : ippStsSizeErr;
   }
}
