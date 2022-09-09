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
//        ippsHashGetTag()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcptool.h"

/*F*
//    Name: ippsHashGetTag
//
// Purpose: Compute digest based on current state.
//          Note, that futher digest update is possible
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pTag == NULL
//                               pState == NULL
//    ippStsContextMatchErr      pState->idCtx != idCtxHash
//    ippStsLengthErr            hashSize < tagLen <1
//    ippStsNoErr                no errors
//
// Parameters:
//    pTag     address of the output digest
//    tagLen   length of digest
//    pState   pointer to the SHS state
//
*F*/
IPPFUN(IppStatus, ippsHashGetTag,(Ipp8u* pTag, int tagLen, const IppsHashState* pState))
{
   /* test state pointer and ID */
   IPP_BAD_PTR2_RET(pTag, pState);
   /* test the context */
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxHash), ippStsContextMatchErr);

   {
      /* size of hash */
      int hashSize = cpHashAlgAttr[HASH_ALG_ID(pState)].hashSize;
      if(tagLen<1||hashSize<tagLen) IPP_ERROR_RET(ippStsLengthErr);

      cpComputeDigest(pTag, tagLen, pState);
      return ippStsNoErr;
   }
}
