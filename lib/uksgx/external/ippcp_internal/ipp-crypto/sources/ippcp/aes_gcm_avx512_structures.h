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
//     AES GCM otimized for AVX512 and AVX512-VAES features
//     Internal Definitions
// 
// 
*/

#ifndef __AES_GCM_AVX512_STRUCTURES_H_
#define __AES_GCM_AVX512_STRUCTURES_H_

#include "owndefs.h"
#include "owncp.h"

// declaration of internal structures used for AVX512 AES GCM optimization

/* GCM data structures */
#define GCM_BLOCK_LEN   16

/**
 * @brief holds GCM operation context
 */
struct gcm_context_data {
        /* init, update and finalize context data */
        Ipp8u  aad_hash[GCM_BLOCK_LEN];
        Ipp64u aad_length;
        Ipp64u in_length;
        Ipp8u  partial_block_enc_key[GCM_BLOCK_LEN];
        Ipp8u  orig_IV[GCM_BLOCK_LEN];
        Ipp8u  current_counter[GCM_BLOCK_LEN];
        Ipp64u partial_block_length;
};

/* #define GCM_BLOCK_LEN   16 */
#define GCM_ENC_KEY_LEN 16
#define GCM_KEY_SETS    (15) /*exp key + 14 exp round keys*/

/**
 * @brief holds intermediate key data needed to improve performance
 *
 * gcm_key_data hold internal key information used by gcm128, gcm192 and gcm256.
 */
struct gcm_key_data {
        Ipp8u expanded_keys[GCM_ENC_KEY_LEN * GCM_KEY_SETS];
        /*
        * (HashKey<<1 mod poly), (HashKey^2<<1 mod poly), ...,
        * (Hashkey^48<<1 mod poly)
        */
        Ipp8u shifted_hkey[GCM_ENC_KEY_LEN * 48];
}
#ifdef LINUX
__attribute__((aligned(64)));
#else
;
#endif

#endif // __AES_GCM_AVX512_STRUCTURES_H_
