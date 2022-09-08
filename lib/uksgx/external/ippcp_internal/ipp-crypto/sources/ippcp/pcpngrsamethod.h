/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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
//  Purpose:
//     Cryptography Primitive.
//     Internal Definitions and
//     Internal ng RSA methods
*/

#if !defined(_CP_NG_RSA_METHOD_H)
#define _CP_NG_RSA_METHOD_H

#include "pcpngmontexpstuff.h"

/*
// declaration of RSA exponentiation
*/
IPP_OWN_FUNPTR (cpSize, ngBufNum, (int modulusBits))

typedef struct _gsMethod_RSA {
   int loModulusBisize;       // application area (lowew
   int hiModulusBisize;       // and upper)
   ngBufNum  bufferNumFunc;   // pub operation buffer in BNU_CHUNK_T
   ngMontExp expFun;          // exponentiation
   ngMontDualExp dualExpFun;  // dual exponentiation
} gsMethod_RSA;


/* GPR exponentiation */
#define gsMethod_RSA_gpr_public OWNAPI(gsMethod_RSA_gpr_public)
   IPP_OWN_DECL (gsMethod_RSA*, gsMethod_RSA_gpr_public, (void))
#define gsMethod_RSA_gpr_private OWNAPI(gsMethod_RSA_gpr_private)
   IPP_OWN_DECL (gsMethod_RSA*, gsMethod_RSA_gpr_private, (void))

/* SSE2 exponentiation */
#if (_IPP>=_IPP_W7)
#define gsMethod_RSA_sse2_public OWNAPI(gsMethod_RSA_sse2_public)
   IPP_OWN_DECL (gsMethod_RSA*, gsMethod_RSA_sse2_public, (void))
#define gsMethod_RSA_sse2_private OWNAPI(gsMethod_RSA_sse2_private)
   IPP_OWN_DECL (gsMethod_RSA*, gsMethod_RSA_sse2_private, (void))
#endif /* _IPP_W7 */

/* AVX2 exponentiation */
#if (_IPP32E>=_IPP32E_L9)
#define gsMethod_RSA_avx2_public OWNAPI(gsMethod_RSA_avx2_public)
   IPP_OWN_DECL (gsMethod_RSA*, gsMethod_RSA_avx2_public, (void))
#define gsMethod_RSA_avx2_private OWNAPI(gsMethod_RSA_avx2_private)
   IPP_OWN_DECL (gsMethod_RSA*, gsMethod_RSA_avx2_private, (void))
#endif /* _IPP32E_L9 */

/* AVX512 exponentiation */
#if (_IPP32E>=_IPP32E_K1)
#define gsMethod_RSA_avx512_public OWNAPI(gsMethod_RSA_avx512_public)
   IPP_OWN_DECL (gsMethod_RSA*, gsMethod_RSA_avx512_public, (void))
#define gsMethod_RSA_avx512_private OWNAPI(gsMethod_RSA_avx512_private)
   IPP_OWN_DECL (gsMethod_RSA*, gsMethod_RSA_avx512_private, (void))
#define gsMethod_RSA_avx512_crt_private OWNAPI(gsMethod_RSA_avx512_crt_private)
   IPP_OWN_DECL (gsMethod_RSA*, gsMethod_RSA_avx512_crt_private, (int privExpBitSize))
#endif /* _IPP32E_K1 */

#endif /* _CP_NG_RSA_METHOD_H */
