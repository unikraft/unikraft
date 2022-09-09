/*******************************************************************************
* Copyright 2014-2021 Intel Corporation
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
//     General Functionality
// 
//  Contents:
//        ippsHashFinal()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcptool.h"


/*F*
//    Name: ippsHashFinal
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
IPPFUN(IppStatus, ippsHashFinal,(Ipp8u* pMD, IppsHashState* pState))
{
   /* test state pointer and ID */
   IPP_BAD_PTR2_RET(pMD, pState);
   /* test the context */
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxHash), ippStsContextMatchErr);

   {
      IppHashAlgId algID = HASH_ALG_ID(pState);
      int hashSize = cpHashAlgAttr[algID].hashSize;

      cpComputeDigest(pMD, hashSize, pState);
      cpReInitHash(pState, algID);

      return ippStsNoErr;
   }
}
