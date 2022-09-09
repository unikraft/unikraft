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
//        ippsGFpECESInit_SM2()
//
*/

#include "pcpgfpecessm2.h"
#include "pcpgfpecstuff.h"

/*F*
//    Name: ippsGFpECESInit_SM2
//
// Purpose: Inits the SM2 algorithm state
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pState == NULL / pEC == NULL
//    ippStsContextMatchErr      pEC, pPoint invalid context
//    ippStsNotSupportedModeErr  pGFE->extdegree > 1
//    ippStsSizeErr              size of the provided state is less than needed
//    ippStsNoErr                no errors
//
// Parameters:
//    pEC              Pointer to an EC to calculate a shared secret size
//    pState           Pointer to a SM2 algorithm state buffer
//    avaliableCtxSize Count of avaliable bytes in the context allocation
//
*F*/
IPPFUN(IppStatus, ippsGFpECESInit_SM2, (IppsGFpECState* pEC, IppsECESState_SM2* pState, int avaliableCtxSize)) {
   IPP_BAD_PTR2_RET(pEC, pState);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);
   IPP_BADARG_RET(!pEC->subgroup, ippStsContextMatchErr);
   IPP_BADARG_RET(1 < pEC->pGF->pGFE->extdegree, ippStsNotSupportedModeErr);

   {
      int realCtxSize;
      ippsGFpECESGetSize_SM2(pEC, &realCtxSize);
      IPP_BADARG_RET(avaliableCtxSize < realCtxSize, ippStsSizeErr);

      {
         int sm3size;
         ippsHashGetSize_rmf(&sm3size);

         ECES_SM2_SET_ID(pState);
         pState->sharedSecretLen = BITS2WORD8_SIZE(pEC->pGF->pGFE->modBitLen) * 2;
         pState->pSharedSecret = ((Ipp8u*)pState) + sizeof(IppsECESState_SM2);
         pState->pKdfHasher = (IppsHashState_rmf*)(((Ipp8u*)pState) + sizeof(IppsECESState_SM2) + pState->sharedSecretLen);
         pState->pTagHasher = (IppsHashState_rmf*)(((Ipp8u*)pState) + sizeof(IppsECESState_SM2) + pState->sharedSecretLen + sm3size);

         ippsHashInit_rmf(pState->pKdfHasher, ippsHashMethod_SM3());

         pState->state = ECESAlgoInit;

         return ippStsNoErr;
      }
   }
}
