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
//        ippsHashInit_rmf()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsHashInit_rmf
//
// Purpose: Init Hash state.
//
// Returns:                Reason:
//    ippStsNullPtrErr           pState == NULL
//                               pMethod == NULL
//    ippStsNoErr                no errors
//
// Parameters:
//    pState   pointer to the Hash state
//    pMethod  hash method
//
*F*/
IPPFUN(IppStatus, ippsHashInit_rmf,(IppsHashState_rmf* pState, const IppsHashMethod* pMethod))
{
   /* test ctx pointers */
   IPP_BAD_PTR2_RET(pState, pMethod);

   PadBlock(0, pState, sizeof(IppsHashState_rmf));
   HASH_METHOD(pState) = pMethod;
   HASH_SET_ID(pState, idCtxHash);
   pMethod->hashInit(HASH_VALUE(pState));
   return ippStsNoErr;
}
