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

#if !defined(_AES_CFB_AESNI_MB)
#define _AES_CFB_AESNI_MB

#include "owndefs.h"
#include "owncp.h"

#if (_IPP32E>=_IPP32E_Y8)

#define aes_cfb16_enc_aesni_mb4 OWNAPI(aes_cfb16_enc_aesni_mb4)
    IPP_OWN_DECL (void, aes_cfb16_enc_aesni_mb4, (const Ipp8u* const source_pa[4], Ipp8u* const dst_pa[4], const int len[4], const int num_of_rounds, const Ipp32u* enc_keys[4], const Ipp8u* pIV[4]))

#endif

#endif /* _AES_CFB_AESNI_MB */
