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
//        ippsDLPUnpack()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"
#include "pcptool.h"

/*F*
//    Name: ippsDLPUnpack
//
// Purpose: Unpack buffer content into the initialized context.
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
IPPFUN(IppStatus, ippsDLPUnpack,(const Ipp8u* pBuffer, IppsDLPState* pDL))
{
   /* test pointers */
   IPP_BAD_PTR2_RET(pDL, pBuffer);

   cpUnpackDLPCtx(pBuffer, pDL);
   DLP_SET_ID(pDL);
   
   return ippStsNoErr;
}
