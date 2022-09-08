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

#if !defined(_AES_CFB_VAES_MB)
#define _AES_CFB_VAES_MB

#include "owndefs.h"
#include "owncp.h"

#if (_IPP32E>=_IPP32E_K1)

#define TRANSPOSE_4x4_I128(X0_, X1_ ,X2_ ,X3_) {\
    __m512i T0_ = _mm512_maskz_shuffle_i64x2((__mmask8)0xFF, X0_, X1_, 0b01000100); \
    __m512i T1_ = _mm512_maskz_shuffle_i64x2((__mmask8)0xFF, X2_, X3_, 0b01000100); \
    __m512i T2_ = _mm512_maskz_shuffle_i64x2((__mmask8)0xFF, X0_, X1_, 0b11101110); \
    __m512i T3_ = _mm512_maskz_shuffle_i64x2((__mmask8)0xFF, X2_, X3_, 0b11101110); \
    \
    X0_ = _mm512_maskz_shuffle_i64x2((__mmask8)0xFF, T0_, T1_, 0b10001000); \
    X1_ = _mm512_maskz_shuffle_i64x2((__mmask8)0xFF, T0_, T1_, 0b11011101); \
    X2_ = _mm512_maskz_shuffle_i64x2((__mmask8)0xFF, T2_, T3_, 0b10001000); \
    X3_ = _mm512_maskz_shuffle_i64x2((__mmask8)0xFF, T2_, T3_, 0b11011101); \
}

#define UPDATE_MASK(len64, mask) {\
    if (len64 < 2 * 4 && len64 >= 0) \
        mask = (__mmask8)(((1 << len64) - 1) & 0xFF); \
    else if (len64 < 0) \
        mask = 0; \
}

#define aes_cfb16_enc_vaes_mb4 OWNAPI(aes_cfb16_enc_vaes_mb4)
    IPP_OWN_DECL (void, aes_cfb16_enc_vaes_mb4, (const Ipp8u* const source_pa[4], Ipp8u* const dst_pa[4], const int len[4], const int num_of_rounds, const Ipp32u* enc_keys[4], const Ipp8u* pIV[4]))
#define aes_cfb16_enc_vaes_mb8 OWNAPI(aes_cfb16_enc_vaes_mb8)
    IPP_OWN_DECL (void, aes_cfb16_enc_vaes_mb8, (const Ipp8u* const source_pa[8], Ipp8u* const dst_pa[8], const int len[8], const int num_of_rounds, const Ipp32u* enc_keys[8], const Ipp8u* pIV[8]))
#define aes_cfb16_enc_vaes_mb16 OWNAPI(aes_cfb16_enc_vaes_mb16)
    IPP_OWN_DECL (void, aes_cfb16_enc_vaes_mb16, (const Ipp8u* const source_pa[16], Ipp8u* const dst_pa[16], const int len[16], const int num_of_rounds, const Ipp32u* enc_keys[16], const Ipp8u* pIV[16]))

#endif

#endif /* _AES_CFB_VAES_MB */
