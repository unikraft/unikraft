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

#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)

#ifndef __SMS4_SBOX_Y8_H_
#define __SMS4_SBOX_Y8_H_

#include "owndefs.h"
#include "owncp.h"

static __ALIGN16 Ipp8u inpMaskLO[] = {0x65,0x41,0xfd,0xd9,0x0a,0x2e,0x92,0xb6,0x0f,0x2b,0x97,0xb3,0x60,0x44,0xf8,0xdc};
static __ALIGN16 Ipp8u inpMaskHI[] = {0x00,0xc9,0x67,0xae,0x80,0x49,0xe7,0x2e,0x4a,0x83,0x2d,0xe4,0xca,0x03,0xad,0x64};
static __ALIGN16 Ipp8u outMaskLO[] = {0xd3,0x59,0x38,0xb2,0xcc,0x46,0x27,0xad,0x36,0xbc,0xdd,0x57,0x29,0xa3,0xc2,0x48};
static __ALIGN16 Ipp8u outMaskHI[] = {0x00,0x50,0x14,0x44,0x89,0xd9,0x9d,0xcd,0xde,0x8e,0xca,0x9a,0x57,0x07,0x43,0x13};

static __ALIGN16 Ipp8u encKey[]    = {0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63};
static __ALIGN16 Ipp8u maskSrows[] = {0x00,0x0d,0x0a,0x07,0x04,0x01,0x0e,0x0b,0x08,0x05,0x02,0x0f,0x0c,0x09,0x06,0x03};

static __ALIGN16 Ipp8u lowBits4[]  = {0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f};

static __ALIGN16 Ipp8u swapBytes[] = {3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12};

static __ALIGN16 Ipp8u affineIn[] = { 0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34, 0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34 };
static __ALIGN16 Ipp8u affineOut[] = { 0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7, 0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7 };

#define M128(mem)    (*((__m128i*)((Ipp8u*)(mem))))

/*
//
// AES and SMS4 ciphers both based on composite field GF(2^8).
// This affine transformation transforms 16 bytes 
// from SMS4 representation to AES representation or vise versa 
// depending on passed masks.
//
*/

__FORCEINLINE __m128i affine(__m128i x, __m128i maskLO, __m128i maskHI)
{
   __m128i T1 = _mm_and_si128(_mm_srli_epi64(x, 4), M128(lowBits4));
   __m128i T0 = _mm_and_si128(x, M128(lowBits4));
   T0 = _mm_shuffle_epi8(maskLO, T0);
   T1 = _mm_shuffle_epi8(maskHI, T1);
   return _mm_xor_si128(T0, T1);
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

__FORCEINLINE __m128i sBox(__m128i block)
{
   block = affine(block, M128(inpMaskLO), M128(inpMaskHI));
   block = _mm_aesenclast_si128(block, M128(encKey));
   block = _mm_shuffle_epi8(block, M128(maskSrows));
   block = affine(block, M128(outMaskLO), M128(outMaskHI));
   
   return block;
}

#if (_IPP==_IPP_I0) || (_IPP32E==_IPP32E_N0)
__FORCEINLINE __m128i L(__m128i x)
{
   __m128i T = _mm_slli_epi32(x, 2);
   T = _mm_xor_si128(T, _mm_srli_epi32(x, 30));

   T = _mm_xor_si128(T, _mm_slli_epi32(x, 10));
   T = _mm_xor_si128(T, _mm_srli_epi32(x, 22));

   T = _mm_xor_si128(T, _mm_slli_epi32(x, 18));
   T = _mm_xor_si128(T, _mm_srli_epi32(x, 14));

   T = _mm_xor_si128(T, _mm_slli_epi32(x, 24));
   T = _mm_xor_si128(T, _mm_srli_epi32(x, 8));
   return T;
}
#else
static __ALIGN16 Ipp8u ROL8[] = { 3,0,1,2,  7,4,5,6,  11,8,9,10,  15,12,13,14 };
static __ALIGN16 Ipp8u ROL16[] = { 2,3,0,1,  6,7,4,5,  10,11,8,9,  14,15,12,13 };
static __ALIGN16 Ipp8u ROL24[] = { 1,2,3,0,  5,6,7,4,  9,10,11,8,  13,14,15,12 };

__FORCEINLINE __m128i L(__m128i x)
{
   __m128i rol2 = _mm_xor_si128(_mm_slli_epi32(x, 2), _mm_srli_epi32(x, 30));
   __m128i rol24 = _mm_shuffle_epi8(x, M128(ROL24));
   __m128i rol10 = _mm_shuffle_epi8(rol2, M128(ROL8));
   __m128i rol18 = _mm_shuffle_epi8(rol2, M128(ROL16));
   __m128i R = _mm_xor_si128(rol24, _mm_xor_si128(rol18, _mm_xor_si128(rol2, rol10)));
   return R;
}
#endif

#define TRANSPOSE_INP(K0,K1,K2,K3, T) \
   T  = _mm_unpacklo_epi32(K0, K1); \
   K1 = _mm_unpackhi_epi32(K0, K1); \
   K0 = _mm_unpacklo_epi32(K2, K3); \
   K3 = _mm_unpackhi_epi32(K2, K3); \
   \
   K2 = _mm_unpacklo_epi64(K1, K3); \
   K3 = _mm_unpackhi_epi64(K1, K3); \
   K1 = _mm_unpackhi_epi64(T,  K0); \
   K0 = _mm_unpacklo_epi64(T,  K0)

#define TRANSPOSE_OUT(K0,K1,K2,K3, T) \
   T  = _mm_unpacklo_epi32(K1, K0); \
   K0 = _mm_unpackhi_epi32(K1, K0); \
   K1 = _mm_unpacklo_epi32(K3, K2); \
   K3 = _mm_unpackhi_epi32(K3, K2); \
   \
   K2 = _mm_unpackhi_epi64(K1,  T); \
   T  = _mm_unpacklo_epi64(K1,  T); \
   K1 = _mm_unpacklo_epi64(K3, K0); \
   K0 = _mm_unpackhi_epi64(K3, K0); \
   K3 = T

#endif /* __SMS4_SBOX_Y8_H_ */

#endif /* _IPP_P8, _IPP32E_Y8 */
