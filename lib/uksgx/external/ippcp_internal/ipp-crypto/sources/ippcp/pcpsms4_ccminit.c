/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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
//  Purpose:
//     Cryptography Primitive.
//     SMS4-CCM implementation.
// 
//     Content:
//        ippsSMS4_CCMInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4authccm.h"
#include "pcptool.h"

/*F*
//    Name: ippsSMS4_CCMInit
//
// Purpose: Init SMS4-CCM state.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//    ippStsMemAllocErr       size of buffer is not match fro operation
//    ippStsLengthErr         keyLen < 16
//    ippStsNoErr             no errors
//
// Parameters:
//    pKey        pointer to the secret key
//    keyLen      length of secret key (in bytes)
//    pCtx        pointer to initialized as CCM context
//    ctxSize     available size (in bytes) of buffer above
//
*F*/
IPPFUN(IppStatus, ippsSMS4_CCMInit,(const Ipp8u* pKey, int keyLen,
                                    IppsSMS4_CCMState* pCtx, int ctxSize))
{
   /* test pCtx pointer */
   IPP_BAD_PTR1_RET(pCtx);

   /* test available size of context buffer */
   IPP_BADARG_RET(ctxSize<cpSizeofCtx_SMS4CCM(), ippStsMemAllocErr);

   /* set state ID */
   SMS4CCM_SET_ID(pCtx);

   /* set default message len */
   SMS4CCM_MSGLEN(pCtx) = 0;

   /* set default tag len */
   SMS4CCM_TAGLEN(pCtx) = 4;

   /* init SMS4 by the secret key */
   return ippsSMS4Init(pKey, keyLen, SMS4CCM_CIPHER(pCtx), cpSizeofCtx_SMS4CCM());
}
