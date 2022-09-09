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
//     Set Round Keys for DES/TDES
// 
//  Contents:
//        ippsDESPack()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdes.h"
#include "pcptool.h"

/*F*
//    Name: ippsDESPack
//
// Purpose: Copy initialized context to the buffer.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pBuffer == NULL
//                            pCtx == NULL
//    ippStsContextMatchErr   pCtx is not DES context
//    ippStsNoErr             no errors
//
// Parameters:
//    pCtx        pointer DES spec
//    pBuffer     pointer to the packed context
//
*F*/
IPPFUN(IppStatus, ippsDESPack,(const IppsDESSpec* pCtx, Ipp8u* pBuffer))
{
   /* test pointers */
   IPP_BAD_PTR2_RET(pCtx, pBuffer);
   /* test the context */
   IPP_BADARG_RET(!VALID_DES_ID(pCtx), ippStsContextMatchErr);

   CopyBlock(pCtx, pBuffer, sizeof(IppsDESSpec));
   IppsDESSpec* pCopy = (IppsDESSpec*)pBuffer;
   DES_RESET_ID(pCopy);
   
   return ippStsNoErr;
}
