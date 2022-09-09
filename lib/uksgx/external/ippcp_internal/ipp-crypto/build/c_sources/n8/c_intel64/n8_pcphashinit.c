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
//        ippsHashInit()
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
//    ippStsNullPtrErr        pState == NULL
//    ippStsNoErr             no errors
//
// Parameters:
//    pState      pointer to the SHA1 state
//    hashAlg     identifier of the hash algorithm
//
*F*/

IPPFUN(IppStatus, ippsHashInit,(IppsHashState* pState, IppHashAlgId hashAlg))
{
   /* get algorithm id */
   hashAlg = cpValidHashAlg(hashAlg);
   /* test hash alg */
   IPP_BADARG_RET(ippHashAlg_Unknown==hashAlg, ippStsNotSupportedModeErr);

   /* test ctx pointer */
   IPP_BAD_PTR1_RET(pState);
   /* test hash alg */

   /* set ctx ID */
   HASH_SET_ID(pState, idCtxHash);
   HASH_ALG_ID(pState) = hashAlg;

   /* init context */
   cpInitHash(pState, hashAlg);
   return ippStsNoErr;
}
