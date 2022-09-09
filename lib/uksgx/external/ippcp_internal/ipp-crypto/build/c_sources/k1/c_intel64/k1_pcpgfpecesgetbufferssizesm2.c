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
//        ippsGFpECESGetBuffersSize_SM2()
//
*/

#include "pcpgfpecessm2.h"
#include "pcpgfpecstuff.h"

/*F*
//    Name: ippsGFpECESGetBuffersSize_SM2
//
// Purpose: Returns sizes of used buffers
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pState == NULL if pPublicKeySize != NULL or all the pointers are NULLs
//    ippStsContextMatchErr      pState invalid context if pPublicKeySize != NULL
//    ippStsNoErr                no errors
//
// Parameters:
//    pPublicKeySize   Pointer to write public (x||y) key length in bytes
//    pMaximumTagSize  Pointer to write maximum tag size in bytes
//    pState           Pointer to a state to get pPublicKeySize from
//
*F*/
IPPFUN(IppStatus, ippsGFpECESGetBuffersSize_SM2, (int* pPublicKeySize,
   int* pMaximumTagSize, const IppsECESState_SM2* pState)) {
   IPP_BADARG_RET(pPublicKeySize == NULL && pMaximumTagSize == NULL && pState == NULL, ippStsNullPtrErr);

   if (pMaximumTagSize)
      *pMaximumTagSize = IPP_SM3_DIGEST_BITSIZE / BYTESIZE;
   if (pPublicKeySize) {
      IPP_BAD_PTR1_RET(pState);
      IPP_BADARG_RET(!VALID_ECES_SM2_ID(pState), ippStsContextMatchErr);
      *pPublicKeySize = pState->sharedSecretLen;
   }

   return ippStsNoErr;
}
