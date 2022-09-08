/*******************************************************************************
* Copyright 2005-2021 Intel Corporation
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
//     RC4 Internal Definitions and Function Prototypes
//
*/

#if !defined(_ARCFOUR_H)
#define _ARCFOUR_H
#define _ARCFOUR_H

#if ((_IPP==_IPP_W7) || (_IPP==_IPP_T7))
   #define rc4word   Ipp8u


#elif ((_IPP>=_IPP_V8) || (_IPP32E>=_IPP32E_M7))
   #define rc4word   Ipp32u

#else
   #define rc4word   Ipp8u
#endif

/*
// ARCFOUR context
*/
struct _cpARCfour {
   Ipp32u      idCtx;      /* RC4 identifier  */

   Ipp32u      cntX;       /* algorithm's counter x  */
   Ipp32u      cntY;       /* algorithm's counter y  */
   rc4word     Sbox[256];  /* current state block.*/
   Ipp8u       Sbox0[256]; /* initial state block */
};

/* alignment */
#define RC4_ALIGNMENT ((int)(sizeof(Ipp32u)))

/*
// Useful macros
*/
#define RC4_SET_ID(ctx)    ((ctx)->idCtx = (Ipp32u)idCtxARCFOUR ^ (Ipp32u)IPP_UINT_PTR(ctx))
#define RC4_RESET_ID(ctx)  ((ctx)->idCtx = (Ipp32u)idCtxARCFOUR)
#define RC4_CNTX(ctx)      ((ctx)->cntX)
#define RC4_CNTY(ctx)      ((ctx)->cntY)
#define RC4_SBOX(ctx)      ((ctx)->Sbox)
#define RC4_SBOX0(ctx)     ((ctx)->Sbox0)

#define RC4_VALID_ID(ctx)  ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((ctx))) == (Ipp32u)idCtxARCFOUR)

/*
// internal functions
*/
#define ARCFourProcessData OWNAPI(ARCFourProcessData)
   IPP_OWN_DECL (void, ARCFourProcessData, (const Ipp8u *pSrc, Ipp8u *pDst, int length, IppsARCFourState *pCtx))

#endif /* _ARCFOUR_H */
