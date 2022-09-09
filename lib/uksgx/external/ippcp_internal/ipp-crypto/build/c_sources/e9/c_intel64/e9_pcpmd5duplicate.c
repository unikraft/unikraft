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
//     Digesting message according to MD5
//     (derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm)
// 
//     Equivalent code is available from RFC 1321.
// 
//  Contents:
//        ippsMD5Duplicate()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsMD5Duplicate
//
// Purpose: Clone MD5 state.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSrcState == NULL
//                            pDstState == NULL
//    ippStsContextMatchErr   pSrcState->idCtx != idCtxMD5
//                            pDstState->idCtx != idCtxMD5
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrcState   pointer to the source MD5 state
//    pDstState   pointer to the target MD5 state
//
// Note:
//    pDstState may to be uninitialized by ippsMD5Init()
//
*F*/
IPPFUN(IppStatus, ippsMD5Duplicate,(const IppsMD5State* pSrcState, IppsMD5State* pDstState))
{
   /* test state pointers */
   IPP_BAD_PTR2_RET(pSrcState, pDstState);
   IPP_BADARG_RET(!HASH_VALID_ID(pSrcState, idCtxMD5), ippStsContextMatchErr);

   /* copy state */
   CopyBlock(pSrcState, pDstState, sizeof(IppsMD5State));
   HASH_SET_ID(pDstState, idCtxMD5);

   return ippStsNoErr;
}
