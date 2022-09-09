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
//     AES-GCM
// 
//  Contents:
//        ippsAES_GCMReset()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if(_IPP32E>=_IPP32E_K0)
#include "pcpaesauthgcm_avx512.h"
#else
#include "pcpaesauthgcm.h"
#endif /* #if(_IPP32E>=_IPP32E_K0) */

/*F*
//    Name: ippsAES_GCMReset
//
// Purpose: Resets AES_GCM context.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pState== NULL
//    ippStsContextMatchErr   pState points on invalid context
//    ippStsNoErr             no errors
//
// Parameters:
//    pState       pointer to the context
//
*F*/
IPPFUN(IppStatus, ippsAES_GCMReset,(IppsAES_GCMState* pState))
{
   /* test pState pointer */
   IPP_BAD_PTR1_RET(pState);

   /* use aligned context */
   pState = (IppsAES_GCMState*)( IPP_ALIGNED_PTR(pState, AESGCM_ALIGNMENT) );
   /* test context validity */
   IPP_BADARG_RET(!AESGCM_VALID_ID(pState), ippStsContextMatchErr);

   /* reset GCM */
   AESGCM_STATE(pState) = GcmInit;
   AESGCM_IV_LEN(pState) = CONST_64(0);
   AESGCM_AAD_LEN(pState) = CONST_64(0);
   AESGCM_TXT_LEN(pState) = CONST_64(0);

   AESGCM_BUFLEN(pState) = 0;
   PadBlock(0, AESGCM_COUNTER(pState), BLOCK_SIZE);
   PadBlock(0, AESGCM_ECOUNTER(pState), BLOCK_SIZE);
   PadBlock(0, AESGCM_ECOUNTER0(pState), BLOCK_SIZE);
   PadBlock(0, AESGCM_GHASH(pState), BLOCK_SIZE);

   #if(_IPP32E>=_IPP32E_K0)

   PadBlock(0, (void*)&AES_GCM_CONTEXT_DATA(pState), sizeof(struct gcm_context_data));

   #endif /* #if(_IPP32E>=_IPP32E_K0) */

   return ippStsNoErr;
}
