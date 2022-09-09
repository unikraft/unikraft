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
//     Digesting message according to SHA256
// 
//  Contents:
//        ippsSHA224Duplicate()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsSHA224Duplicate
//
// Purpose: Clone SHA224 state.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSrcState == NULL
//                            pDstState == NULL
//    ippStsContextMatchErr   pSrcState->idCtx != idCtxSHA256
//                            pDstState->idCtx != idCtxSHA256
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrcState   pointer to the source SHA224 state
//    pDstState   pointer to the target SHA224 state
//
// Note:
//    pDstState may to be uninitialized by ippsSHA224Init()
//
*F*/

IPPFUN(IppStatus, ippsSHA224Duplicate,(const IppsSHA224State* pSrcState, IppsSHA224State* pDstState))
{
   return ippsSHA256Duplicate(pSrcState, pDstState);
}
