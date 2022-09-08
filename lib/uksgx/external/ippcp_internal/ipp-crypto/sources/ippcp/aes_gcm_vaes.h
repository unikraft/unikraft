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
//     AES GCM AVX512-VAES
//     Internal Functions Prototypes
// 
*/

#ifndef __AES_GCM_VAES_H_
#define __AES_GCM_VAES_H_

#include "owndefs.h"
#include "owncp.h"

#include "aes_gcm_avx512_structures.h"

#if(_IPP32E>=_IPP32E_K0)

#define aes_gcm_enc_128_update_vaes_avx512 OWNAPI(aes_gcm_enc_128_update_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_enc_128_update_vaes_avx512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, Ipp8u *out, const Ipp8u *in, Ipp64u len))
#define aes_gcm_enc_192_update_vaes_avx512 OWNAPI(aes_gcm_enc_192_update_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_enc_192_update_vaes_avx512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, Ipp8u *out, const Ipp8u *in, Ipp64u len))
#define aes_gcm_enc_256_update_vaes_avx512 OWNAPI(aes_gcm_enc_256_update_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_enc_256_update_vaes_avx512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, Ipp8u *out, const Ipp8u *in, Ipp64u len))

#define aes_gcm_dec_128_update_vaes_avx512 OWNAPI(aes_gcm_dec_128_update_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_dec_128_update_vaes_avx512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, Ipp8u *out, const Ipp8u *in, Ipp64u len))
#define aes_gcm_dec_192_update_vaes_avx512 OWNAPI(aes_gcm_dec_192_update_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_dec_192_update_vaes_avx512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, Ipp8u *out, const Ipp8u *in, Ipp64u len))
#define aes_gcm_dec_256_update_vaes_avx512 OWNAPI(aes_gcm_dec_256_update_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_dec_256_update_vaes_avx512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, Ipp8u *out, const Ipp8u *in, Ipp64u len))

#define aes_gcm_gettag_128_vaes_avx512 OWNAPI(aes_gcm_gettag_128_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_gettag_128_vaes_avx512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, Ipp8u *auth_tag, Ipp64u auth_tag_len))
#define aes_gcm_gettag_192_vaes_avx512 OWNAPI(aes_gcm_gettag_192_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_gettag_192_vaes_avx512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, Ipp8u *auth_tag, Ipp64u auth_tag_len))
#define aes_gcm_gettag_256_vaes_avx512 OWNAPI(aes_gcm_gettag_256_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_gettag_256_vaes_avx512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, Ipp8u *auth_tag, Ipp64u auth_tag_len))

#define aes_gcm_precomp_128_vaes_avx512 OWNAPI(aes_gcm_precomp_128_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_precomp_128_vaes_avx512, (struct gcm_key_data *key_data))
#define aes_gcm_precomp_192_vaes_avx512 OWNAPI(aes_gcm_precomp_192_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_precomp_192_vaes_avx512, (struct gcm_key_data *key_data))
#define aes_gcm_precomp_256_vaes_avx512 OWNAPI(aes_gcm_precomp_256_vaes_avx512)
    IPP_OWN_DECL (void, aes_gcm_precomp_256_vaes_avx512, (struct gcm_key_data *key_data))

#define aes_gcm_aad_hash_update_vaes512 OWNAPI(aes_gcm_aad_hash_update_vaes512)
    IPP_OWN_DECL (void, aes_gcm_aad_hash_update_vaes512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, const Ipp8u *aad, const Ipp64u aad_len))
#define aes_gcm_aad_hash_finalize_vaes512 OWNAPI(aes_gcm_aad_hash_finalize_vaes512)
    IPP_OWN_DECL (void, aes_gcm_aad_hash_finalize_vaes512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, const Ipp8u *aad, const Ipp64u aad_len, const Ipp64u aad_general_len))

#define aes_gcm_iv_hash_update_vaes512 OWNAPI(aes_gcm_iv_hash_update_vaes512)
    IPP_OWN_DECL (void, aes_gcm_iv_hash_update_vaes512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, const Ipp8u *iv, const Ipp64u iv_len))
#define aes_gcm_iv_hash_finalize_vaes512 OWNAPI(aes_gcm_iv_hash_finalize_vaes512)
    IPP_OWN_DECL (void, aes_gcm_iv_hash_finalize_vaes512, (const struct gcm_key_data *key_data, struct gcm_context_data *context_data, const Ipp8u *iv, const Ipp64u iv_len, const Ipp64u iv_general_len))

#endif /* #if(_IPP32E>=_IPP32E_K0) */

#endif
