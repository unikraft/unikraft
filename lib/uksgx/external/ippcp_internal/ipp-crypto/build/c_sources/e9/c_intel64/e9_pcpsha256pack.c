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
//        ippsSHA256Pack()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsSHA256Pack
//
// Purpose: Copy initialized context to the buffer.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pState == NULL
//                            pBuffer == NULL
//    ippStsNoErr             no errors
//
// Parameters:
//    pState      pointer to the hash state
//    pBuffer     pointer to the destination buffer
//
*F*/
IPPFUN(IppStatus, ippsSHA256Pack,(const IppsSHA256State* pState, Ipp8u* pBuffer))
{
   /* test pointers */
   IPP_BAD_PTR2_RET(pState, pBuffer);
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxSHA256), ippStsContextMatchErr);

   CopyBlock(pState, pBuffer, sizeof(IppsSHA256State));
   IppsSHA256State* pCopy = (IppsSHA256State*)pBuffer;
   HASH_RESET_ID(pCopy, idCtxSHA256);
   
   return ippStsNoErr;
}
