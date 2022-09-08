/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
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
//     SMS4 encryption/decryption
// 
//  Contents:
//     affine()
//     sBox()
//     L()
//     TRANSPOSE_INP()
//     TRANSPOSE_OUT()
// 
*/

#if (_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9)

#ifndef __SMS4_SBOX_L9_H_
#define __SMS4_SBOX_L9_H_

#include "owndefs.h"
#include "owncp.h"

static __ALIGN32 Ipp8u inpMaskLO[] = {0x65,0x41,0xfd,0xd9,0x0a,0x2e,0x92,0xb6,0x0f,0x2b,0x97,0xb3,0x60,0x44,0xf8,0xdc,
                                      0x65,0x41,0xfd,0xd9,0x0a,0x2e,0x92,0xb6,0x0f,0x2b,0x97,0xb3,0x60,0x44,0xf8,0xdc};
static __ALIGN32 Ipp8u inpMaskHI[] = {0x00,0xc9,0x67,0xae,0x80,0x49,0xe7,0x2e,0x4a,0x83,0x2d,0xe4,0xca,0x03,0xad,0x64,
                                      0x00,0xc9,0x67,0xae,0x80,0x49,0xe7,0x2e,0x4a,0x83,0x2d,0xe4,0xca,0x03,0xad,0x64};
static __ALIGN32 Ipp8u outMaskLO[] = {0xd3,0x59,0x38,0xb2,0xcc,0x46,0x27,0xad,0x36,0xbc,0xdd,0x57,0x29,0xa3,0xc2,0x48,
                                      0xd3,0x59,0x38,0xb2,0xcc,0x46,0x27,0xad,0x36,0xbc,0xdd,0x57,0x29,0xa3,0xc2,0x48};
static __ALIGN32 Ipp8u outMaskHI[] = {0x00,0x50,0x14,0x44,0x89,0xd9,0x9d,0xcd,0xde,0x8e,0xca,0x9a,0x57,0x07,0x43,0x13,
                                      0x00,0x50,0x14,0x44,0x89,0xd9,0x9d,0xcd,0xde,0x8e,0xca,0x9a,0x57,0x07,0x43,0x13};

static __ALIGN32 Ipp8u maskSrows[] = {0x00,0x0d,0x0a,0x07,0x04,0x01,0x0e,0x0b,0x08,0x05,0x02,0x0f,0x0c,0x09,0x06,0x03,
                                      0x00,0x0d,0x0a,0x07,0x04,0x01,0x0e,0x0b,0x08,0x05,0x02,0x0f,0x0c,0x09,0x06,0x03};

static __ALIGN16 Ipp8u encKey[] = {0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63};

static __ALIGN32 Ipp8u lowBits4[] = {0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
                                     0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f};

static __ALIGN32 Ipp8u swapBytes[] = {3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12,
                                      3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12};

static __ALIGN32 Ipp8u permMask[] = {0x00,0x00,0x00,0x00, 0x04,0x00,0x00,0x00, 0x01,0x00,0x00,0x00, 0x05,0x00,0x00,0x00,
                                     0x02,0x00,0x00,0x00, 0x06,0x00,0x00,0x00, 0x03,0x00,0x00,0x00, 0x07,0x00,0x00,0x00};

static __ALIGN32 Ipp8u affineIn[] = { 0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34, 0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34,
                                    0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34, 0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34 };
static __ALIGN32 Ipp8u affineOut[] = { 0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7, 0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7,
                                       0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7, 0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7 };

#define M256(mem)    (*((__m256i*)((Ipp8u*)(mem))))
#define M128(mem)    (*((__m128i*)((Ipp8u*)(mem))))

/*
//
// AES and SMS4 ciphers both based on composite field GF(2^8).
// This affine transformation transforms 16 bytes 
// from SMS4 representation to AES representation or vise versa 
// depending on passed masks.
//
*/

__FORCEINLINE __m256i affine(__m256i x, __m256i maskLO, __m256i maskHI)
{
   __m256i T1 = _mm256_and_si256(_mm256_srli_epi64(x, 4), M256(lowBits4));
   __m256i T0 = _mm256_and_si256(x, M256(lowBits4));
   T0 = _mm256_shuffle_epi8(maskLO, T0);
   T1 = _mm256_shuffle_epi8(maskHI, T1);
   return _mm256_xor_si256(T0, T1);
}

__FORCEINLINE __m256i AES_ENC_LAST(__m256i x, __m128i key)
{
   __m128i t0 = _mm256_extracti128_si256(x, 0);
   __m128i t1 = _mm256_extracti128_si256(x, 1);
   t0 = _mm_aesenclast_si128(t0, key);
   t1 = _mm_aesenclast_si128(t1, key);
   x = _mm256_inserti128_si256(x, t0, 0);
   x = _mm256_inserti128_si256(x, t1, 1);
   return x;
}

/*
//
// GF(256) is isomorfic.
// Encoding/decoding data of SM4 and AES are elements of GF(256).
// The difference in representation only.
// (It happend due to using different generating polynomials in SM4 and AES representations).
// Doing data conversion from SM4 to AES domain
// lets use AES specific intrinsics to perform less expensive SMS4 S-box computation.
// 
// Original SMS4 S-box algorithm is converted to the following:
//
// - transform  data  from  SMS4  representation  to AES representation
// - compute S-box  value using  _mm_aesenclast_si128  with special key 
// - re-shuffle data  after _mm_aesenclast_si128 that shuffle it inside
// - transform data back from AES representation to SMS4 representation
//
*/

__FORCEINLINE __m256i sBox(__m256i block)
{
   block = affine(block, M256(inpMaskLO), M256(inpMaskHI));
   block = AES_ENC_LAST(block, M128(encKey));
   block = _mm256_shuffle_epi8(block, M256(maskSrows));
   block = affine(block, M256(outMaskLO), M256(outMaskHI));

   return block;
}

__FORCEINLINE __m256i L(__m256i x)
{
   __m256i T = _mm256_xor_si256(_mm256_slli_epi32(x, 2), _mm256_srli_epi32(x,30));

   T = _mm256_xor_si256(T, _mm256_slli_epi32 (x,10));
   T = _mm256_xor_si256(T, _mm256_srli_epi32 (x,22));

   T = _mm256_xor_si256(T, _mm256_slli_epi32 (x,18));
   T = _mm256_xor_si256(T, _mm256_srli_epi32 (x,14));

   T = _mm256_xor_si256(T, _mm256_slli_epi32 (x,24));
   T = _mm256_xor_si256(T, _mm256_srli_epi32 (x, 8));
   return T;
}

/*
// inp: T0, T1, T2, T3
// out: K0, K1, K2, K3
*/
#define TRANSPOSE_INP(K0,K1,K2,K3, T0,T1,T2,T3) \
   K0 = _mm256_unpacklo_epi32(T0, T1); \
   K1 = _mm256_unpacklo_epi32(T2, T3); \
   K2 = _mm256_unpackhi_epi32(T0, T1); \
   K3 = _mm256_unpackhi_epi32(T2, T3); \
   \
   T0 = _mm256_unpacklo_epi64(K0, K1); \
   T1 = _mm256_unpacklo_epi64(K2, K3); \
   T2 = _mm256_unpackhi_epi64(K0, K1); \
   T3 = _mm256_unpackhi_epi64(K2, K3); \
   \
   K2 = _mm256_permutevar8x32_epi32(T1, M256(permMask)); \
   K1 = _mm256_permutevar8x32_epi32(T2, M256(permMask)); \
   K3 = _mm256_permutevar8x32_epi32(T3, M256(permMask)); \
   K0 = _mm256_permutevar8x32_epi32(T0, M256(permMask))

/*
// inp: K0, K1, K2, K3
// out: T0, T1, T2, T3
*/
#define TRANSPOSE_OUT(T0,T1,T2,T3, K0,K1,K2,K3) \
   T0 = _mm256_unpacklo_epi32(K1, K0); \
   T1 = _mm256_unpacklo_epi32(K3, K2); \
   T2 = _mm256_unpackhi_epi32(K1, K0); \
   T3 = _mm256_unpackhi_epi32(K3, K2); \
   \
   K0 = _mm256_unpacklo_epi64(T1, T0); \
   K1 = _mm256_unpacklo_epi64(T3, T2); \
   K2 = _mm256_unpackhi_epi64(T1, T0); \
   K3 = _mm256_unpackhi_epi64(T3, T2); \
   \
   T0 = _mm256_permute2x128_si256(K0, K2, 0x20); \
   T1 = _mm256_permute2x128_si256(K1, K3, 0x20); \
   T2 = _mm256_permute2x128_si256(K0, K2, 0x31); \
   T3 = _mm256_permute2x128_si256(K1, K3, 0x31)

#endif /* __SMS4_SBOX_L9_H_ */

#endif /* _IPP_G9, _IPP32E_L9 */
