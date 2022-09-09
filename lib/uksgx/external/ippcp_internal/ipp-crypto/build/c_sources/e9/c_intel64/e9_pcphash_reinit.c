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
//        cpReInitHash()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcptool.h"

/*F*
//    Name: ippsHashInit
//
// Purpose: Init Hash state.
//
// Returns:                Reason:
//    ippStsNullPtrErr           pState == NULL
//    ippStsNotSupportedModeErr  if algID is not match to supported hash alg
//    ippStsNoErr                no errors
//
// Parameters:
//    pCtx     pointer to the Hash state
//    algID    hash alg ID
//
*F*/
IPP_OWN_DEFN (int, cpReInitHash, (IppsHashState* pCtx, IppHashAlgId algID))
{
   int hashIvSize = cpHashIvSize(algID);
   const Ipp8u* iv = cpHashIV[algID];

   HASH_LENLO(pCtx) = CONST_64(0);
   HASH_LENHI(pCtx) = CONST_64(0);
   HAHS_BUFFIDX(pCtx) = 0;
   CopyBlock(iv, HASH_VALUE(pCtx), hashIvSize);

   return hashIvSize;
}
