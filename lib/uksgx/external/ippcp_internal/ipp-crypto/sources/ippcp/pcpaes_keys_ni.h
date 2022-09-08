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

#if !defined (_PCP_AES_KEYS_NI_H)
#define _PCP_AES_KEYS_NI_H

#if (_AES_NI_ENABLING_==_FEATURE_ON_) || (_AES_NI_ENABLING_==_FEATURE_TICKTOCK_)

//////////////////////////////////////////////////////////////////////
#define cpExpandAesKey_NI OWNAPI(cpExpandAesKey_NI)
    IPP_OWN_DECL (void, cpExpandAesKey_NI, (const Ipp8u* pSecret, IppsAESSpec* pCtx))
#define aes_DecKeyExpansion_NI OWNAPI(aes_DecKeyExpansion_NI)
    IPP_OWN_DECL (void, aes_DecKeyExpansion_NI, (Ipp8u* decKeys, const Ipp8u* encKeys, int nr))
#define aes128_KeyExpansion_NI OWNAPI(aes128_KeyExpansion_NI)
    IPP_OWN_DECL (void, aes128_KeyExpansion_NI, (Ipp8u* keyExp, const Ipp8u* userkey))
#define aes192_KeyExpansion_NI OWNAPI(aes192_KeyExpansion_NI)
    IPP_OWN_DECL (void, aes192_KeyExpansion_NI, (Ipp8u* keyExp, const Ipp8u* userkey))
#define aes256_KeyExpansion_NI OWNAPI(aes256_KeyExpansion_NI)
    IPP_OWN_DECL (void, aes256_KeyExpansion_NI, (Ipp8u* keyExp, const Ipp8u* userkey))

#endif /* #if (_AES_NI_ENABLING_==_FEATURE_ON_) || (_AES_NI_ENABLING_==_FEATURE_TICKTOCK_) */

#endif /* #if !defined (_PCP_AES_KEYS_NI_H) */
