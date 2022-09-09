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
//        ippsGFpECESEncrypt_SM2()
//
*/

#include "pcpgfpecessm2.h"
#include "pcpgfpecstuff.h"

/*F*
//    Name: ippsGFpECESEncrypt_SM2
//
// Purpose: Encrypts the given buffer, updates the auth tag
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pInput == NULL / pOutput == NULL / pState == NULL
//    ippStsContextMatchErr      pState invalid context or the algorithm is in an invalid state
//    ippStsSizeErr              dataLen < 0
//    ippStsNoErr                no errors
//
// Parameters:
//    pInput          Pointer to input data
//    pOutput         Pointer to output data
//    dataLen         Size of input and output buffers
//    pState          Pointer to a SM2 algorithm state
//
*F*/
IPPFUN(IppStatus, ippsGFpECESEncrypt_SM2, (const Ipp8u* pInput, Ipp8u* pOutput, int dataLen, IppsECESState_SM2* pState)) {
   IPP_BAD_PTR3_RET(pInput, pOutput, pState);
   IPP_BADARG_RET(!VALID_ECES_SM2_ID(pState), ippStsContextMatchErr);
   /* a shared secret should be computed and the process should not be finished by getTag */
   IPP_BADARG_RET(pState->state != ECESAlgoProcessing, ippStsIncompleteContextErr);
   IPP_BADARG_RET(dataLen < 0, ippStsSizeErr);

   ippsHashUpdate_rmf(pInput, dataLen, pState->pTagHasher);
   {
      int i;
      for (i = 0; i < dataLen; ++i) {
         pOutput[i] = pInput[i] ^ cpECES_SM2KdfNextByte(pState);
      }
   }

   return ippStsNoErr;
}
