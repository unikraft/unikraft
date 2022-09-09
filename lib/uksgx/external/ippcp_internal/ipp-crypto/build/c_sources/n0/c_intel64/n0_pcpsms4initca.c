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
//     Initialization of SM4S
// 
//  Contents:
//        ippsSMS4Init()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

/*F*
//    Name: ippsSMS4Init
//
// Purpose: Init SMS4 context for future usage
//          and setup secret key.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//    ippStsMemAllocErr       size of buffer is not match for operation
//    ippStsLengthErr         keyLen<16
//    ippStsContextMatchErr   !VALID_SMS4_ID(pCtx)
//    ippStsNoErr             no errors
//
// Parameters:
//    pKey        secret key
//    keyLen      length of the secret key (in bytes)
//    pCtx        pointer to buffer initialized as AES context
//    ctxSize     available size (in bytes) of buffer above
//
// Note:
//    if pKey==NULL, then SMS4 initialized by zero value key
//
*F*/
IPPFUN(IppStatus, ippsSMS4Init,(const Ipp8u* pKey, int keyLen,
                                IppsSMS4Spec* pCtx, int ctxSize))
{
   /* test context pointer */
   IPP_BAD_PTR1_RET(pCtx);

   /* test available size of context buffer */
   IPP_BADARG_RET(ctxSize<cpSizeofCtx_SMS4(), ippStsMemAllocErr);

   /* make sure in legal keyLen */
   IPP_BADARG_RET(keyLen<16, ippStsLengthErr);

   /* setup context ID */
   SMS4_SET_ID(pCtx);
   /* compute round keys */
   return ippsSMS4SetKey(pKey, keyLen, pCtx);
}
