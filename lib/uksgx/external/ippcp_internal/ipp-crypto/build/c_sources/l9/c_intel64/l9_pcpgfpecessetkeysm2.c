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
//        ippsGFpECESSetKey_SM2()
//
*/

#include "pcpgfpecessm2.h"
#include "pcpgfpecstuff.h"

/*F*
//    Name: ippsGFpECESSetKey_SM2
//
// Purpose: Resets all counters, computes shared secret and saves it into the SM2 algorithm state
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pPrivate == NULL / pPublic == NULL / pState == NULL / pEC == NULL
//    ippStsContextMatchErr      the algorithm is in an invalid state or any of the specified contexts does not match the operation
//    ippStsOutOfRangeErr        private key does not belong to the EC's finite field or public key/result does not belong to EC
//    ippStsNotSupportedModeErr  pGFE->extdegree > 1
//    ippStsBadArgErr            curve element size is not the same as in init / pPrivate is negative / pPrivate > pEC GFp mod
//    ippStsPointAtInfinity      shared secret is a point at infinity
//    ippStsNoErr                no errors
//
// Parameters:
//    pEC               Pointer to an EC to calculate a shared secret size
//    pState            Pointer to a SM2 algorithm state buffer
//    pPrivate          Pointer to a private component of shared secret
//    pPublic           Pointer to a public component
//    pEcScratchBuffer  Pointer to a scratch buffer for computations on the EC
//
*F*/
IPPFUN(IppStatus, ippsGFpECESSetKey_SM2, (const IppsBigNumState* pPrivate,
   const IppsGFpECPoint* pPublic, IppsECESState_SM2* pState,
   IppsGFpECState* pEC, Ipp8u* pEcScratchBuffer)) {
   IPP_BAD_PTR4_RET(pPrivate, pPublic, pState, pEC);
   IPP_BADARG_RET(!VALID_ECES_SM2_ID(pState), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);
   IPP_BADARG_RET(!pEC->subgroup, ippStsContextMatchErr);
   IPP_BADARG_RET(1 < pEC->pGF->pGFE->extdegree, ippStsNotSupportedModeErr);

   {
      gsModEngine* pGFE = pEC->pGF->pGFE;
      /* curve element size is not the same */
      IPP_BADARG_RET(BITS2WORD8_SIZE(pGFE->modBitLen) * 2 != pState->sharedSecretLen, ippStsBadArgErr);

      {
         IppStatus multResult;
         IppsGFpECPoint PT;
         IppsGFpElement ptX, ptY;
         int finitePoint = 0;

         cpEcGFpInitPoint(&PT, cpEcGFpGetPool(1, pEC), 0, pEC);
         multResult = ippsGFpECMulPoint(pPublic, pPrivate, &PT, pEC, pEcScratchBuffer);
         if (ippStsNoErr == multResult) {
            cpGFpElementConstruct(&ptX, cpGFpGetPool(1, pGFE), pGFE->modLen);
            cpGFpElementConstruct(&ptY, cpGFpGetPool(1, pGFE), pGFE->modLen);
            finitePoint = gfec_GetPoint(ptX.pData, ptY.pData, &PT, pEC);
            if (finitePoint) {
               ippsGFpGetElementOctString(&ptX, pState->pSharedSecret, pState->sharedSecretLen / 2, pEC->pGF);
               ippsGFpGetElementOctString(&ptY, pState->pSharedSecret + pState->sharedSecretLen / 2, pState->sharedSecretLen / 2, pEC->pGF);

               pState->kdfCounter = 0;
               pState->kdfIndex = IPP_SM3_DIGEST_BITSIZE / BYTESIZE; /* will generate a kdf window */
               pState->wasNonZero = 0;
               pState->state = ECESAlgoKeySet;
            }
            cpGFpReleasePool(2, pGFE); /* release ptX and ptY from the pool */
         }
         cpEcGFpReleasePool(1, pEC); /* release PT from the pool */

         if (multResult)
            return multResult;
         return finitePoint ? ippStsNoErr : ippStsPointAtInfinity;
      }
   }
}
