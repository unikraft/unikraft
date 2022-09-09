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
//        ippsDESUnpack()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdes.h"
#include "pcptool.h"

/*F*
//    Name: ippsDESUnpack
//
// Purpose: Unpack buffer content into the initialized context.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pBuffer == NULL
//                            pCtx == NULL
//    ippStsNoErr             no errors
//
// Parameters:
//    pCtx        pointer DES spec
//    pBuffer     pointer to the packed context
//
*F*/
IPPFUN(IppStatus, ippsDESUnpack,(const Ipp8u* pBuffer, IppsDESSpec* pCtx))
{
   /* test pointers */
   IPP_BAD_PTR2_RET(pCtx, pBuffer);

   CopyBlock(pBuffer, pCtx, sizeof(IppsDESSpec));
   DES_SET_ID(pCtx);

   return ippStsNoErr;
}
