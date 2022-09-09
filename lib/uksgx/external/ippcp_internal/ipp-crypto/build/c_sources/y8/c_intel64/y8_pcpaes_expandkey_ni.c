/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
//     Cryptography Primitive. AES keys expansion
// 
//  Contents:
//        cpExpandAesKey_NI()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcpaes_keys_ni.h"

#if (_AES_NI_ENABLING_==_FEATURE_ON_) || (_AES_NI_ENABLING_==_FEATURE_TICKTOCK_)

IPP_OWN_DEFN (void, cpExpandAesKey_NI, (const Ipp8u* pSecret, IppsAESSpec* pCtx))
{
   int nRounds = RIJ_NR(pCtx);
   Ipp8u* pEncKeys = RIJ_EKEYS(pCtx);
   Ipp8u* pDecKeys = RIJ_DKEYS(pCtx);

   switch (nRounds) {
   case 12: aes192_KeyExpansion_NI(pEncKeys, pSecret);  break;
   case 14: aes256_KeyExpansion_NI(pEncKeys, pSecret);  break;
   default: aes128_KeyExpansion_NI(pEncKeys, pSecret);  break;
   }

   aes_DecKeyExpansion_NI(pDecKeys, pEncKeys, nRounds);
}

#endif /* #if (_AES_NI_ENABLING_==_FEATURE_ON_) || (_AES_NI_ENABLING_==_FEATURE_TICKTOCK_) */

