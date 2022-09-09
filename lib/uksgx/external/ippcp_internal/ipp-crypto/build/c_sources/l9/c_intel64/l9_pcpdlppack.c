/*******************************************************************************
* Copyright 2005-2021 Intel Corporation
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
//     DL over Prime Field (initialization)
// 
//  Contents:
//        ippsDLPPack()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"
#include "pcptool.h"

/*F*
//    Name: ippsDLPPack
//
// Purpose: Copy initialized context to the buffer.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSize == NULL
//                            pCtx == NULL
//    ippStsNoErr             no errors
//
// Parameters:
//    pCtx        pointer DLP ctx
//    pSize       pointer to the packed spec size
//
*F*/
IPPFUN(IppStatus, ippsDLPPack,(const IppsDLPState* pDL, Ipp8u* pBuffer))
{
   /* test pointers */
   IPP_BAD_PTR2_RET(pDL, pBuffer);
   /* test the context */
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   cpPackDLPCtx(pDL, pBuffer);
   IppsDLPState* pCopy = (IppsDLPState*)pBuffer;
   DLP_RESET_ID(pCopy);

   return ippStsNoErr;
}
