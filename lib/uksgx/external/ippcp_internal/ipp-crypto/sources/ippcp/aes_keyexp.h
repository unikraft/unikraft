/*******************************************************************************
* Copyright 2020-2021 Intel Corporation
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
//     AES Key Expansion
//     Internal Definitions
// 
*/

#ifndef __AES_KEYEXP_H_
#define __AES_KEYEXP_H_

#include "owndefs.h"
#include "owncp.h"

#include "pcpaesauthgcm_avx512.h"

// These functions for key expansion are used only with AVX512 and AVX512-VAES optimizations for AES GCM
// TODO: replase AVX2 keyexp with AVX512 keyexp
#if(_IPP32E>=_IPP32E_K0)

#define aes_keyexp_128_enc OWNAPI(aes_keyexp_128_enc)
    IPP_OWN_DECL (void, aes_keyexp_128_enc, (const Ipp8u* key, struct gcm_key_data *key_data))
#define aes_keyexp_192_enc OWNAPI(aes_keyexp_192_enc)
    IPP_OWN_DECL (void, aes_keyexp_192_enc, (const Ipp8u* key, struct gcm_key_data *key_data))
#define aes_keyexp_256_enc OWNAPI(aes_keyexp_256_enc)
    IPP_OWN_DECL (void, aes_keyexp_256_enc, (const Ipp8u* key, struct gcm_key_data *key_data))

#endif // (_IPP32E>=_IPP32E_K0)

#endif // __AES_KEYEXP_H_
