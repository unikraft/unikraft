/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
//     Security Hash Standard
//     Generalized Functionality
// 
//  Contents:
//        ippsHashFinal_rmf()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash_rmf.h"
#include "pcptool.h"


/*F*
//    Name: ippsHashFinal_rmf
//
// Purpose: Complete message digesting and return digest.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pMD == NULL
//                               pState == NULL
//    ippStsContextMatchErr      pState->idCtx != idCtxHash
//    ippStsNoErr                no errors
//
// Parameters:
//    pMD     address of the output digest
//    pState  pointer to the SHS state
//
*F*/
IPPFUN(IppStatus, ippsHashFinal_rmf,(Ipp8u* pMD, IppsHashState_rmf* pState))
{
   /* test state pointer and ID */
   IPP_BAD_PTR2_RET(pMD, pState);
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxHash), ippStsContextMatchErr);

   {
      const IppsHashMethod* method = HASH_METHOD(pState);

      cpFinalize_rmf(HASH_VALUE(pState),
                     HASH_BUFF(pState), HAHS_BUFFIDX(pState),
                     HASH_LENLO(pState), HASH_LENHI(pState),
                     method);
      /* convert hash into oct string */
      method->hashOctStr(pMD, HASH_VALUE(pState));

      /* re-init hash value */
      HAHS_BUFFIDX(pState) = 0;
      HASH_LENLO(pState) = 0;
      HASH_LENHI(pState) = 0;
      method->hashInit(HASH_VALUE(pState));

      return ippStsNoErr;
   }
}
