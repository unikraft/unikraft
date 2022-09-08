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
//     sBox()
//     L()
//     TRANSPOSE_INP()
//     TRANSPOSE_OUT()
//
*/

#ifndef __SMS4_SBOX_GFNI512_H_
#define __SMS4_SBOX_GFNI512_H_

#include "owndefs.h"
#include "owncp.h"

#if (_IPP32E>=_IPP32E_K1)

#if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)

static __ALIGN64 Ipp8u swapBytes[] = { 3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12,
                                      3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12,
                                      3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12,
                                      3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12 };
/*
// Not used in current pipeline
static __ALIGN32 Ipp8u permMask256[] = {0x00,0x00,0x00,0x00, 0x04,0x00,0x00,0x00, 0x01,0x00,0x00,0x00, 0x05,0x00,0x00,0x00,
                                        0x02,0x00,0x00,0x00, 0x06,0x00,0x00,0x00, 0x03,0x00,0x00,0x00, 0x07,0x00,0x00,0x00};
*/

static __ALIGN64 Ipp8u permMask_in[]  = {0,0x00,0x00,0x00, 4,0x00,0x00,0x00, 8,0x00,0x00,0x00, 12,0x00,0x00,0x00,
                                         1,0x00,0x00,0x00, 5,0x00,0x00,0x00, 9,0x00,0x00,0x00, 13,0x00,0x00,0x00,
                                         2,0x00,0x00,0x00, 6,0x00,0x00,0x00, 10,0x00,0x00,0x00, 14,0x00,0x00,0x00,
                                         3,0x00,0x00,0x00, 7,0x00,0x00,0x00, 11,0x00,0x00,0x00, 15,0x00,0x00,0x00 };

static __ALIGN64 Ipp8u permMask_out[] = {12,0x00,0x00,0x00, 8,0x00,0x00,0x00, 4,0x00,0x00,0x00, 0,0x00,0x00,0x00,
                                         13,0x00,0x00,0x00, 9,0x00,0x00,0x00, 5,0x00,0x00,0x00, 1,0x00,0x00,0x00,
                                         14,0x00,0x00,0x00, 10,0x00,0x00,0x00, 6,0x00,0x00,0x00, 2,0x00,0x00,0x00,
                                         15,0x00,0x00,0x00, 11,0x00,0x00,0x00, 7,0x00,0x00,0x00, 3,0x00,0x00,0x00};

static __ALIGN64 Ipp8u affineIn[] = { 0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34, 0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34,
                                    0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34, 0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34,
                                    0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34, 0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34,
                                    0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34, 0x52,0xBC,0x2D,0x02,0x9E,0x25,0xAC,0x34 };
static __ALIGN64 Ipp8u affineOut[] = { 0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7, 0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7,
                                       0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7, 0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7,
                                       0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7, 0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7,
                                       0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7, 0x19,0x8b,0x6c,0x1e,0x51,0x8e,0x2d,0xd7 };

#define M512(mem)    (*((__m512i*)(mem)))
#define M256(mem)    (*((__m256i*)(mem)))
#define M128(mem)    (*((__m128i*)(mem)))

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
// - compute S-box  value
// - transform data back from AES representation to SMS4 representation
//
*/

/*
// sBox
*/

__FORCEINLINE __m512i sBox512(__m512i block)
{
   block = _mm512_gf2p8affine_epi64_epi8(block, M512(affineIn), 0x65);
   block = _mm512_gf2p8affineinv_epi64_epi8(block, M512(affineOut), 0xd3);
   return block;
}

/* 
// Not used in current pipeline
__FORCEINLINE __m256i sBox256(__m256i block)
{
   block = _mm256_gf2p8affine_epi64_epi8(block, M256(affineIn), 0x65);
   block = _mm256_gf2p8affineinv_epi64_epi8(block, M256(affineOut), 0xd3);
   return block;

}
*/

__FORCEINLINE __m128i sBox128(__m128i block)
{
   block = _mm_gf2p8affine_epi64_epi8(block, M128(affineIn), 0x65);
   block = _mm_gf2p8affineinv_epi64_epi8(block, M128(affineOut), 0xd3);
   return block;

}

/*
// L
*/

__FORCEINLINE __m512i L512(__m512i x)
{
   __m512i rolled0 = _mm512_rol_epi32(x, 2);
   __m512i rolled1 = _mm512_rol_epi32(x, 10);
   __m512i temp    = _mm512_xor_si512(rolled0, rolled1);
   __m512i rolled2 = _mm512_rol_epi32(x, 18);
   __m512i rolled3 = _mm512_rol_epi32(x, 24);
   __m512i res     = _mm512_ternarylogic_epi32(temp, rolled2, rolled3, 0x96);
   return  res;
}

/*
// Not used in current pipeline
__FORCEINLINE __m256i L256(__m256i x)
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
*/

__FORCEINLINE __m128i L128(__m128i x)
{
   __m128i rolled0 = _mm_rol_epi32(x, 2);
   __m128i rolled1 = _mm_rol_epi32(x, 10);
   __m128i temp    = _mm_xor_si128(rolled1, rolled0);
   __m128i rolled2 = _mm_rol_epi32(x, 18);
   __m128i rolled3 = _mm_rol_epi32(x, 24);
   __m128i res     = _mm_ternarylogic_epi32(temp, rolled2, rolled3, 0x96);
   return  res;
}


/*
// TRANSPOSE_INP
*/

/*
// inp: T0, T1, T2, T3
// out: K0, K1, K2, K3
*/
#define TRANSPOSE_INP_512(K0,K1,K2,K3, T0,T1,T2,T3) \
   K0 = _mm512_unpacklo_epi32(T0, T1); \
   K1 = _mm512_unpacklo_epi32(T2, T3); \
   K2 = _mm512_unpackhi_epi32(T0, T1); \
   K3 = _mm512_unpackhi_epi32(T2, T3); \
   \
   T0 = _mm512_unpacklo_epi64(K0, K1); \
   T1 = _mm512_unpacklo_epi64(K2, K3); \
   T2 = _mm512_unpackhi_epi64(K0, K1); \
   T3 = _mm512_unpackhi_epi64(K2, K3); \
   \
   K2 = _mm512_permutexvar_epi32(M512(permMask_in), T1); \
   K1 = _mm512_permutexvar_epi32(M512(permMask_in), T2); \
   K3 = _mm512_permutexvar_epi32(M512(permMask_in), T3); \
   K0 = _mm512_permutexvar_epi32(M512(permMask_in), T0)

/*
// inp: T0, T1, T2, T3
// out: K0, K1, K2, K3
*/

/*
// Not used in current pipeline
#define TRANSPOSE_INP_256(K0,K1,K2,K3, T0,T1,T2,T3) \
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
   K2 = _mm256_permutevar8x32_epi32(T1, M256(permMask256)); \
   K1 = _mm256_permutevar8x32_epi32(T2, M256(permMask256)); \
   K3 = _mm256_permutevar8x32_epi32(T3, M256(permMask256)); \
   K0 = _mm256_permutevar8x32_epi32(T0, M256(permMask256))
*/

#define TRANSPOSE_INP_128(K0,K1,K2,K3, T) \
   T  = _mm_unpacklo_epi32(K0, K1); \
   K1 = _mm_unpackhi_epi32(K0, K1); \
   K0 = _mm_unpacklo_epi32(K2, K3); \
   K3 = _mm_unpackhi_epi32(K2, K3); \
   \
   K2 = _mm_unpacklo_epi64(K1, K3); \
   K3 = _mm_unpackhi_epi64(K1, K3); \
   K1 = _mm_unpackhi_epi64(T,  K0); \
   K0 = _mm_unpacklo_epi64(T,  K0)

/*
// TRANSPOSE_OUT
*/

/*
// inp: K0, K1, K2, K3
// out: T0, T1, T2, T3
*/

#define TRANSPOSE_OUT_512(T0,T1,T2,T3, K0,K1,K2,K3) \
   T0 = _mm512_shuffle_i32x4(K0, K1, 0x44); \
   T1 = _mm512_shuffle_i32x4(K0, K1, 0xee); \
   T2 = _mm512_shuffle_i32x4(K2, K3, 0x44); \
   T3 = _mm512_shuffle_i32x4(K2, K3, 0xee); \
   \
   K0 = _mm512_shuffle_i32x4(T0, T2, 0x88); \
   K1 = _mm512_shuffle_i32x4(T0, T2, 0xdd); \
   K2 = _mm512_shuffle_i32x4(T1, T3, 0x88); \
   K3 = _mm512_shuffle_i32x4(T1, T3, 0xdd); \
   \
   K0 = _mm512_permutexvar_epi32(M512(permMask_out), K0);\
   K1 = _mm512_permutexvar_epi32(M512(permMask_out), K1);\
   K2 = _mm512_permutexvar_epi32(M512(permMask_out), K2);\
   K3 = _mm512_permutexvar_epi32(M512(permMask_out), K3);\
   \
T0=K0,T1=K1,T2=K2,T3=K3


/*
// inp: K0, K1, K2, K3
// out: T0, T1, T2, T3
*/

/*
// Not used in current pipeline
#define TRANSPOSE_OUT_256(T0,T1,T2,T3, K0,K1,K2,K3) \
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
*/

#define TRANSPOSE_OUT_128(K0,K1,K2,K3, T) \
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

//#define PR(X) printf("%08u %08u %08u %08u | %08u %08u %08u %08u | %08u %08u %08u %08u | %08u %08u %08u %08u\n",\
//         X.m512i_u32[0], X.m512i_u32[1], X.m512i_u32[2],\
//         X.m512i_u32[3], X.m512i_u32[4], X.m512i_u32[5],\
//         X.m512i_u32[6], X.m512i_u32[7], X.m512i_u32[8],\
//         X.m512i_u32[9], X.m512i_u32[10], X.m512i_u32[11],\
//         X.m512i_u32[12], X.m512i_u32[13], X.m512i_u32[14],\
//         X.m512i_u32[15]);

#endif

#endif /* if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */

#endif /* __SMS4_SBOX_GFNI512_H_ */
