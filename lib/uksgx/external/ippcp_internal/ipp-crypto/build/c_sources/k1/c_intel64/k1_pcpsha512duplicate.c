/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
//     SHA512 message digest
// 
//  Contents:
//        ippsSHA512Duplicate()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsSHA512Duplicate
//
// Purpose: Clone SHA512 state.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSrcState == NULL
//                            pDstState == NULL
//    ippStsContextMatchErr   pSrcState->idCtx != idCtxSHA512
//                            pDstState->idCtx != idCtxSHA512
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrcState   pointer to the source SHA512 state
//    pDstState   pointer to the target SHA512 state
// Note:
//    pDstState may to be uninitialized by ippsSHA512Init()
//
*F*/
IPPFUN(IppStatus, ippsSHA512Duplicate,(const IppsSHA512State* pSrcState, IppsSHA512State* pDstState))
{
   /* test state pointers */
   IPP_BAD_PTR2_RET(pSrcState, pDstState);
   /* test states ID */
   IPP_BADARG_RET(!HASH_VALID_ID(pSrcState, idCtxSHA512), ippStsContextMatchErr);

   /* copy state */
   CopyBlock(pSrcState, pDstState, sizeof(IppsSHA512State));
   HASH_SET_ID(pDstState, idCtxSHA512);

   return ippStsNoErr;
}
