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
//        ippsRSAVerifyHash_PKCS1v15_rmf()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpngrsa.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

#include "pcprsa_pkcs1c15_data.h"
#include "pcprsa_verifysign_pkcs1v15.h"


#if defined( _ABL_ )

IPPFUN(IppStatus, ippsRSAVerifyHash_PKCS1v15_rmf,(const Ipp8u* md,
                                                  const Ipp8u* pSign, int* pIsValid,
                                                  const IppsRSAPublicKeyState* pKey,
                                                  const IppsHashMethod* pMethod,
                                                        Ipp8u* pBuffer))
{
   IppHashAlgId hashAlg;

   /* test public key context */
   IPP_BAD_PTR3_RET(pKey, pBuffer, pMethod);
   IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(pKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PUB_KEY_IS_SET(pKey), ippStsIncompleteContextErr);

   /* test hash algorith ID */
   hashAlg = pMethod->hashAlgId;
   IPP_BADARG_RET(ippHashAlg_SM3==hashAlg, ippStsNotSupportedModeErr);

   /* test data pointer */
   IPP_BAD_PTR3_RET(md, pSign, pIsValid);

   *pIsValid = 0;
   return VerifySign(md, pMethod->hashLen,
                     pksc15_salt[hashAlg].pSalt, pksc15_salt[hashAlg].saltLen,
                     pSign, pIsValid,
                     pKey,
                     (BNU_CHUNK_T*)(IPP_ALIGNED_PTR((pBuffer), (int)sizeof(BNU_CHUNK_T))))? ippStsNoErr : ippStsSizeErr;
}

#endif /* #if defined( _ABL_ ) */

