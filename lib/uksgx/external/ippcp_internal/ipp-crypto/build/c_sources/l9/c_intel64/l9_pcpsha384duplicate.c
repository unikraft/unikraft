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
//        ippsSHA384Duplicate()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsSHA384Duplicate
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
//    pSrcState   pointer to the source SHA384 state
//    pDstState   pointer to the target SHA384 state
// Note:
//    pDstState may to be uninitialized by ippsSHA384Init()
//
*F*/

IPPFUN(IppStatus, ippsSHA384Duplicate,(const IppsSHA384State* pSrcState, IppsSHA384State* pDstState))
{
   return ippsSHA512Duplicate(pSrcState, pDstState);
}
