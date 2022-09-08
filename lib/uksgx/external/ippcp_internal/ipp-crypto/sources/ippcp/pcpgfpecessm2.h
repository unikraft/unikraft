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
//     ES encryption/decryption API
//
//
*/

#if !defined(_CP_GFP_ES_SM2_H)
#define _CP_GFP_ES_SM2_H

#include "owncp.h"

typedef enum {
   ECESAlgoInit,
   ECESAlgoKeySet,
   ECESAlgoProcessing,
   ECESAlgoFinished
} ECESAlgoState;

struct _cpStateECES_SM2 {
   Ipp32u idCtx;
   Ipp8u* pSharedSecret;
   Ipp32s sharedSecretLen;

   ECESAlgoState state;

   Ipp32u kdfCounter;
   Ipp8u pKdfWindow[IPP_SM3_DIGEST_BITSIZE / BYTESIZE];
   Ipp8u wasNonZero;
   Ipp8u kdfIndex;

   IppsHashState_rmf* pKdfHasher;
   IppsHashState_rmf* pTagHasher;
};

#define ECES_SM2_SET_ID(stt)   ((stt)->idCtx = (Ipp32u)idxCtxECES_SM2 ^ (Ipp32u)IPP_UINT_PTR(stt))
#define VALID_ECES_SM2_ID(stt) ((((stt)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((stt))) == (Ipp32u)idxCtxECES_SM2)

/* get a byte, update 0-kdf status */
__INLINE Ipp8u cpECES_SM2KdfNextByte(IppsECESState_SM2* pState) {
   if (pState->kdfIndex == IPP_SM3_DIGEST_BITSIZE / BYTESIZE) {
      ++pState->kdfCounter;
      pState->kdfIndex = 0;

      {
         Ipp8u ctnStr[sizeof(Ipp32u)];
         ippsHashUpdate_rmf(pState->pSharedSecret, pState->sharedSecretLen, pState->pKdfHasher);
         U32_TO_HSTRING(ctnStr, pState->kdfCounter);
         ippsHashUpdate_rmf(ctnStr, sizeof(Ipp32u), pState->pKdfHasher);
         ippsHashFinal_rmf(pState->pKdfWindow, pState->pKdfHasher);
      }
   }

   pState->wasNonZero |= pState->pKdfWindow[pState->kdfIndex];

   return pState->pKdfWindow[pState->kdfIndex++];
}

#endif
