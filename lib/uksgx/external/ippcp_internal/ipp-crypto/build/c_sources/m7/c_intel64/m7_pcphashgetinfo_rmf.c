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
//        ippsHashMethodGetInfo()
//        ippsHashGetInfo_rmf()
//
*/

#include "owncp.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsHashMethodGetInfo
//
// Purpose: Returns info of the Hash algorithm
//
// Returns:                   Reason:
//    ippStsNullPtrErr              NULL == pMethod
//                                  NULL == pInfo
//
//    ippStsNoErr                   no error
//
// Parameters:
//    pInfo      Pointer to the info structure
//    pMethod    Pointer to the IppsHashMethod
//
*F*/
IPPFUN(IppStatus, ippsHashMethodGetInfo,(IppsHashInfo* pInfo, const IppsHashMethod* pMethod))
{
   /* test pointers */
   IPP_BAD_PTR2_RET(pInfo, pMethod);

   pInfo->hashSize = pMethod->hashLen;
   pInfo->msgBlockSize = pMethod->msgBlkSize;

   return ippStsNoErr;
}

/*F*
// Name: ippsHashGetInfo_rmf
//
// Purpose: Returns info of the using Hash
//
// Returns:                   Reason:
//    ippStsNullPtrErr              NULL == pState
//                                  NULL == pInfo
//
//    ippStsContextMatchErr         invalid pState->idCtx
//
//    ippStsNoErr                   no error
//
// Parameters:
//    pInfo      Pointer to the info structure
//    pState     Pointer to the state context
//
*F*/
IPPFUN(IppStatus, ippsHashGetInfo_rmf,(IppsHashInfo* pInfo, const IppsHashState_rmf* pState))
{
   /* test pointers */
   IPP_BAD_PTR2_RET(pInfo, pState);
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxHash), ippStsContextMatchErr);

   pInfo->hashSize = HASH_METHOD(pState)->hashLen;
   pInfo->msgBlockSize = HASH_METHOD(pState)->msgBlkSize;

   return ippStsNoErr;
}
