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
//        ippsHashDuplicate()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcptool.h"

/*F*
//    Name: ippsHashDuplicate
//
// Purpose: Clone Hash context.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSrcState == NULL
//                            pDstState == NULL
//    ippStsContextMatchErr   pSrcState->idCtx != idCtxHash
//                            pDstState->idCtx != idCtxHash
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrcState     pointer to the source Hash context
//    pDstState     pointer to the target Hash context
//
// Note:
//    pDstState may to be uninitialized by ippsHashInit()
//
*F*/
IPPFUN(IppStatus, ippsHashDuplicate,(const IppsHashState* pSrcState, IppsHashState* pDstState))
{
   /* test state pointers */
   IPP_BAD_PTR2_RET(pSrcState, pDstState);
   /* test states ID */
   IPP_BADARG_RET(!HASH_VALID_ID(pSrcState, idCtxHash), ippStsContextMatchErr);

   /* copy state */
   CopyBlock(pSrcState, pDstState, sizeof(IppsHashState));
   HASH_SET_ID(pDstState, idCtxHash);

   return ippStsNoErr;
}
