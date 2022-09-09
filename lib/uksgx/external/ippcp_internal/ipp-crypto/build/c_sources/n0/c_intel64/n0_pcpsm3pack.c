/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     Digesting message according to SM3
// 
//  Contents:
//        ippsSM3Pack()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsSM3Pack
//
// Purpose: Copy initialized context to the buffer.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSize == NULL
//                            pBuffer == NULL
//    ippStsContextMatchErr   pState->idCtx != idCtxSM3
//    ippStsNoErr             no errors
//
// Parameters:
//    pState      pointer hash state
//    pBuffer     pointer to the destination buffer
//
*F*/
IPPFUN(IppStatus, ippsSM3Pack,(const IppsSM3State* pState, Ipp8u* pBuffer))
{
   /* test pointers */
   IPP_BAD_PTR2_RET(pState, pBuffer);
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxSM3), ippStsContextMatchErr);

   CopyBlock(pState, pBuffer, sizeof(IppsSM3State));
   IppsSM3State* pCopy = (IppsSM3State*)pBuffer;
   HASH_RESET_ID(pCopy, idCtxSM3);
   return ippStsNoErr;
}
