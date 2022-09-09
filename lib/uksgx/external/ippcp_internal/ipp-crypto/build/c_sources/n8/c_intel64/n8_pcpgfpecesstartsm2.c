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
//        ippsGFpECESStart_SM2()
//
*/

#include "pcpgfpecessm2.h"

/*F*
//    Name: ippsGFpECESStart_SM2
//
// Purpose: Starts an SM2 encryption chain.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pState == NULL
//    ippStsContextMatchErr      pState invalid context or the algorithm is in an invalid state
//    ippStsNoErr                no errors
//
// Parameters:
//    pState   Pointer to a SM2 algorithm state
//
*F*/
IPPFUN(IppStatus, ippsGFpECESStart_SM2, (IppsECESState_SM2* pState)) {
   IPP_BAD_PTR1_RET(pState);
   IPP_BADARG_RET(!VALID_ECES_SM2_ID(pState), ippStsContextMatchErr);
   IPP_BADARG_RET(pState->state != ECESAlgoKeySet, ippStsContextMatchErr);

   ippsHashInit_rmf(pState->pTagHasher, ippsHashMethod_SM3());
   ippsHashUpdate_rmf(pState->pSharedSecret, pState->sharedSecretLen / 2, pState->pTagHasher);

   pState->state = ECESAlgoProcessing;

   return ippStsNoErr;
}
