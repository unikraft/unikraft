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

#include "owncp.h"
#include "pcphash_rmf.h"

// Check all the ippsRSASign_PKCS1v15_rmf parameters and align pPrvKey, pPubKey pointers
__INLINE IppStatus SingleSignPkcs1v15RmfPreproc(const Ipp8u* pMsg, int msgLen,
   Ipp8u* pSign,
   const IppsRSAPrivateKeyState** pPrvKey,
   const IppsRSAPublicKeyState**  pPubKey,
   const IppsHashMethod* pMethod,
   Ipp8u* pScratchBuffer)
{
   /* test private key context */
   IPP_BAD_PTR3_RET(*pPrvKey, pScratchBuffer, pMethod);
   IPP_BADARG_RET(!RSA_PRV_KEY_VALID_ID(*pPrvKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PRV_KEY_IS_SET(*pPrvKey), ippStsIncompleteContextErr);

   /* test hash algorithm ID */
   IPP_BADARG_RET(ippHashAlg_Unknown == pMethod->hashAlgId, ippStsNotSupportedModeErr);
   IPP_BADARG_RET(ippHashAlg_SM3 == pMethod->hashAlgId, ippStsNotSupportedModeErr);

   /* use aligned public key context if defined */
   if (*pPubKey) {
      IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(*pPubKey), ippStsContextMatchErr);
      IPP_BADARG_RET(!RSA_PUB_KEY_IS_SET(*pPubKey), ippStsIncompleteContextErr);
   }

   /* test data pointer */
   IPP_BAD_PTR2_RET(pMsg, pSign);
   /* test length */
   IPP_BADARG_RET(msgLen < 0, ippStsLengthErr);

   return ippStsNoErr;
}

// Check all the ippsRSAVerify_PKCS1v15_rmf parameters, set valid=0, align pKey pointer
__INLINE IppStatus SingleVerifyPkcs1v15RmfPreproc(const Ipp8u* pMsg, int msgLen,
   const Ipp8u* pSign, int* pIsValid,
   const IppsRSAPublicKeyState** pKey,
   const IppsHashMethod* pMethod,
   Ipp8u* pScratchBuffer)
{
   /* test public key context */
   IPP_BAD_PTR3_RET(*pKey, pScratchBuffer, pMethod);
   IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(*pKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PUB_KEY_IS_SET(*pKey), ippStsIncompleteContextErr);

   /* test hash algorithm ID */
   IPP_BADARG_RET(ippHashAlg_Unknown == pMethod->hashAlgId, ippStsNotSupportedModeErr);
   IPP_BADARG_RET(ippHashAlg_SM3 == pMethod->hashAlgId, ippStsNotSupportedModeErr);

   /* test data pointer */
   IPP_BAD_PTR3_RET(pMsg, pSign, pIsValid);
   /* test length */
   IPP_BADARG_RET(msgLen < 0, ippStsLengthErr);

   *pIsValid = 0;

   return ippStsNoErr;
}
