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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
// 
//     Context:
//        ippsAES_CCMInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesauthccm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

/*F*
//    Name: ippsAES_CCMInit
//
// Purpose: Init AES-CCM state.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pState == NULL
//    ippStsMemAllocErr       size of buffer is not match fro operation
//    ippStsLengthErr         keyLen != 16 &&
//                                   != 24 &&
//                                   != 32
//    ippStsNoErr             no errors
//
// Parameters:
//    pKey        pointer to the secret key
//    keyLen      length of secret key (in bytes)
//    pState      pointer to initialized as CCM context
//    ctxSize     available size (in bytes) of buffer above
//
*F*/
IPPFUN(IppStatus, ippsAES_CCMInit,(const Ipp8u* pKey, int keyLen,
                                   IppsAES_CCMState* pState, int ctxSize))
{
   /* test pState pointer */
   IPP_BAD_PTR1_RET(pState);

   /* test available size of context buffer */
   IPP_BADARG_RET(ctxSize<cpSizeofCtx_AESCCM(), ippStsMemAllocErr);

   /* set state ID */
   AESCCM_SET_ID(pState);

   /* set default message len*/
   AESCCM_MSGLEN(pState) = 0;

   /* set default tag len*/
   AESCCM_TAGLEN(pState) = 4;

   /* init AES by the secret key */
   return ippsAESInit(pKey, keyLen, AESCCM_CIPHER(pState), cpSizeofCtx_AES());
}
