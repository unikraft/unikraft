/*******************************************************************************
* Copyright 2018-2021 Intel Corporation
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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     EC over GF(p) Operations
//
//     Context:
//        ippsGFpECESFinal_SM2()
//
*/

#include "pcpgfpecessm2.h"
#include "pcpgfpecstuff.h"

/*F*
//    Name: ippsGFpECESFinal_SM2
//
// Purpose: Ends SM2 algorithm chain and returns an auth tag
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pTag == NULL / pState == NULL
//    ippStsContextMatchErr      pState invalid context or the algorithm is in an invalid state
//    ippStsSizeErr              tagLen < 0 || tagLen > IPP_SM3_DIGEST_BITSIZE / BYTESIZE
//    ippStsShareKeyErr          All the kdf provided bytes were 0. The operation is completed successfully, but the warning is given
//    ippStsNoErr                no errors
//
// Parameters:
//    pTag            Pointer to a tag buffer to write to
//    tagLen          Size of the tag [0; IPP_SM3_DIGEST_BITSIZE / BYTESIZE]
//    pState          Pointer to a SM2 algorithm state
//
*F*/
IPPFUN(IppStatus, ippsGFpECESFinal_SM2, (Ipp8u* pTag, int tagLen, IppsECESState_SM2* pState)) {
   IPP_BAD_PTR2_RET(pTag, pState);
   IPP_BADARG_RET(!VALID_ECES_SM2_ID(pState), ippStsContextMatchErr);
   /* a shared secret should be computed and the process should not be finished by getTag */
   IPP_BADARG_RET(pState->state != ECESAlgoProcessing, ippStsIncompleteContextErr);
   IPP_BADARG_RET(tagLen < 0 || tagLen > IPP_SM3_DIGEST_BITSIZE / BYTESIZE, ippStsSizeErr);

   ippsHashUpdate_rmf(pState->pSharedSecret + pState->sharedSecretLen / 2, pState->sharedSecretLen / 2, pState->pTagHasher);
   if (tagLen == IPP_SM3_DIGEST_BITSIZE / BYTESIZE) {
      ippsHashFinal_rmf(pTag, pState->pTagHasher);
   } else {
      Ipp8u pFinal[IPP_SM3_DIGEST_BITSIZE / BYTESIZE];
      int i;
      ippsHashFinal_rmf(pFinal, pState->pTagHasher);
      for (i = 0; i < tagLen; ++i) {
         pTag[i] = pFinal[i];
      }
   }

   pState->state = ECESAlgoFinished; /* cannot proceed futher due to closing ippsSM3Update */

   /* do the operation, but return an error code in 0-case */
   return pState->wasNonZero ? ippStsNoErr : ippStsShareKeyErr;
}
