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
//        ippsHashPack_rmf()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsHashPack_rmf
//
// Purpose: Copy initialized context to the buffer.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pState == NULL
//                            pBuffer == NULL
//    ippStsContextMatchErr   pState->idCtx != idCtxHash
//    ippStsNoMemErr          bufSize < sizeof(IppsHashState_rmf)
//    ippStsNoErr             no errors
//
// Parameters:
//    pState      pointer hash state
//    pBuffer     pointer to the destination buffer
//    bufSize     size of the destination buffer
//
*F*/
IPPFUN(IppStatus, ippsHashPack_rmf,(const IppsHashState_rmf* pState, Ipp8u* pBuffer, int bufSize))
{
   /* test pointers */
   IPP_BAD_PTR2_RET(pState, pBuffer);
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxHash), ippStsContextMatchErr);
   /* test buffer length */
   IPP_BADARG_RET((int)(sizeof(IppsHashState_rmf))>bufSize, ippStsNoMemErr);

   CopyBlock(pState, pBuffer, sizeof(IppsHashState_rmf));
   IppsHashState_rmf* pCopy = (IppsHashState_rmf*)pBuffer;
   HASH_RESET_ID(pCopy, idCtxHash);
   return ippStsNoErr;
}
