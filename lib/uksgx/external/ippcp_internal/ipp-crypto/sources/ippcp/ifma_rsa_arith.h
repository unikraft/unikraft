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

#ifndef _IFMA_RSA_ARITH_H_
#define _IFMA_RSA_ARITH_H_

/*
 * RSA kernels implemented with AVX512_IFMA256 ISA
 */

#include "owncp.h"

#if(_IPP32E>=_IPP32E_K1)

/* Almost Montgomery Multiplication / Squaring */
#define ifma256_amm52x20 OWNAPI(ifma256_amm52x20)
  IPP_OWN_DECL(void, ifma256_amm52x20, (Ipp64u out[20], const Ipp64u a[20], const Ipp64u b[20], const Ipp64u m[20], Ipp64u k0))
#define ifma256_ams52x20 OWNAPI(ifma256_ams52x20)
  IPP_OWN_DECL(void, ifma256_ams52x20, (Ipp64u out[20], const Ipp64u a[20], const Ipp64u m[20], Ipp64u k0))
#define ifma256_amm52x30 OWNAPI(ifma256_amm52x30)
  IPP_OWN_DECL(void, ifma256_amm52x30, (Ipp64u out[32], const Ipp64u a[32], const Ipp64u b[32], const Ipp64u m[32], Ipp64u k0))
#define ifma256_ams52x30 OWNAPI(ifma256_ams52x30)
  IPP_OWN_DECL(void, ifma256_ams52x30, (Ipp64u out[32], const Ipp64u a[32], const Ipp64u m[32], Ipp64u k0))
#define ifma256_amm52x40 OWNAPI(ifma256_amm52x40)
  IPP_OWN_DECL(void, ifma256_amm52x40, (Ipp64u out[40], const Ipp64u a[40], const Ipp64u b[40], const Ipp64u m[40], Ipp64u k0))
#define ifma256_ams52x40 OWNAPI(ifma256_ams52x40)
  IPP_OWN_DECL(void, ifma256_ams52x40, (Ipp64u out[40], const Ipp64u a[40], const Ipp64u m[40], Ipp64u k0))


/*
 * Dual Almost Montgomery Multiplication / Squaring
 * (two independent operations and data arrays)
 */
#define ifma256_amm52x20_dual OWNAPI(ifma256_amm52x20_dual)
  IPP_OWN_DECL(void, ifma256_amm52x20_dual, (Ipp64u out[2][20], const Ipp64u a[2][20], const Ipp64u b[2][20], const Ipp64u m[2][20], const Ipp64u k0[2]))
#define ifma256_ams52x20_dual OWNAPI(ifma256_ams52x20_dual)
  IPP_OWN_DECL(void, ifma256_ams52x20_dual, (Ipp64u out[2][20], const Ipp64u a[2][20], const Ipp64u m[2][20], const Ipp64u k0[2]))
#define ifma256_amm52x30_dual OWNAPI(ifma256_amm52x30_dual)
  IPP_OWN_DECL(void, ifma256_amm52x30_dual, (Ipp64u out[2][32], const Ipp64u a[2][32], const Ipp64u b[2][32], const Ipp64u m[2][32], const Ipp64u k0[2]))
#define ifma256_ams52x30_dual OWNAPI(ifma256_ams52x30_dual)
  IPP_OWN_DECL(void, ifma256_ams52x30_dual, (Ipp64u out[2][32], const Ipp64u a[2][32], const Ipp64u m[2][32], const Ipp64u k0[2]))
#define ifma256_amm52x40_dual OWNAPI(ifma256_amm52x40_dual)
  IPP_OWN_DECL(void, ifma256_amm52x40_dual, (Ipp64u out[2][40], const Ipp64u a[2][40], const Ipp64u b[2][40], const Ipp64u m[2][40], const Ipp64u k0[2]))
#define ifma256_ams52x40_dual OWNAPI(ifma256_ams52x40_dual)
  IPP_OWN_DECL(void, ifma256_ams52x40_dual, (Ipp64u out[2][40], const Ipp64u a[2][40], const Ipp64u m[2][40], const Ipp64u k0[2]))


/* Exponentiation */
#define ifma256_exp52x20 OWNAPI(ifma256_exp52x20)
  IPP_OWN_DECL (void, ifma256_exp52x20, (Ipp64u *out,
                                   const Ipp64u *base,
                                   const Ipp64u *exp,
                                   const Ipp64u *modulus,
                                   const Ipp64u *toMont,
                                   const Ipp64u k0))

/* Dual exponentiation */
#define ifma256_exp52x20_dual OWNAPI(ifma256_exp52x20_dual)
  IPP_OWN_DECL (void, ifma256_exp52x20_dual, (Ipp64u out    [2][20],
                                        const Ipp64u base   [2][20],
                                        const Ipp64u *exp   [2], // 2x16
                                        const Ipp64u modulus[2][20],
                                        const Ipp64u toMont [2][20],
                                        const Ipp64u k0     [2]))

#define ifma256_exp52x30_dual OWNAPI(ifma256_exp52x30_dual)
  IPP_OWN_DECL (void, ifma256_exp52x30_dual, (Ipp64u out    [2][32],
                                        const Ipp64u base   [2][32],
                                        const Ipp64u *exp   [2], // 2x24
                                        const Ipp64u modulus[2][32],
                                        const Ipp64u toMont [2][32],
                                        const Ipp64u k0     [2]))

#define ifma256_exp52x40_dual OWNAPI(ifma256_exp52x40_dual)
  IPP_OWN_DECL (void, ifma256_exp52x40_dual, (Ipp64u out    [2][40],
                                        const Ipp64u base   [2][40],
                                        const Ipp64u *exp   [2], // 2x32
                                        const Ipp64u modulus[2][40],
                                        const Ipp64u toMont [2][40],
                                        const Ipp64u k0     [2]))

#endif // #if(_IPP32E>=_IPP32E_K1)
#endif // #ifndef _IFMA_RSA_ARITH_H_
