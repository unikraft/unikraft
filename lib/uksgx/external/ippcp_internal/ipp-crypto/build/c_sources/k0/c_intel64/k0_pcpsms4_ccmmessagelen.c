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
//        ippsSMS4_CCMMessageLen()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4authccm.h"
#include "pcptool.h"

/*F*
//    Name: ippsSMS4_CCMMessageLen
//
// Purpose: Setup expected length of payload (in bytes).
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//    ippStsContextMatchErr   !VALID_SMS4CCM_ID()
//    ippStsNoErr             no errors
//
// Parameters:
//    msgLen   length in bytes of the message expected to be processed
//    pCtx      pointer to the CCM context
//
*F*/
IPPFUN(IppStatus, ippsSMS4_CCMMessageLen,(Ipp64u msgLen, IppsSMS4_CCMState* pCtx))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!VALID_SMS4CCM_ID(pCtx), ippStsContextMatchErr);

   /* init for new message */
   SMS4CCM_MSGLEN(pCtx) = msgLen;

   return ippStsNoErr;
}
