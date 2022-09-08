/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
//     Internal Definitions and
//     Internal Pseudo Random Generator Function Prototypes
// 
*/

#if !defined(_CP_PRNG_H)
#define _CP_PRNG_H

/*
// Pseudo-random generation context
*/

#define MAX_XKEY_SIZE       512
#define DEFAULT_XKEY_SIZE   512 /* must be >=160 || <=512 */

struct _cpPRNG {
   Ipp32u      idCtx;                                 /* PRNG identifier            */
   cpSize      seedBits;                              /* secret seed-key bitsize    */
   BNU_CHUNK_T Q[BITS_BNU_CHUNK(160)];                /* modulus                    */
   BNU_CHUNK_T T[BITS_BNU_CHUNK(160)];                /* parameter of SHA_G() funct */
   BNU_CHUNK_T xAug[BITS_BNU_CHUNK(MAX_XKEY_SIZE)];   /* optional entropy augment   */
   BNU_CHUNK_T xKey[BITS_BNU_CHUNK(MAX_XKEY_SIZE)];   /* secret seed-key            */
};

/* alignment */
#define PRNG_ALIGNMENT ((int)(sizeof(void*)))

#define RAND_SET_ID(ctx)   ((ctx)->idCtx = (Ipp32u)idCtxPRNG ^ (Ipp32u)IPP_UINT_PTR(ctx))
#define RAND_SEEDBITS(ctx) ((ctx)->seedBits)
#define RAND_Q(ctx)        ((ctx)->Q)
#define RAND_T(ctx)        ((ctx)->T)
#define RAND_XAUGMENT(ctx) ((ctx)->xAug)
#define RAND_XKEY(ctx)     ((ctx)->xKey)

#define RAND_VALID_ID(ctx)  ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((ctx))) == (Ipp32u)idCtxPRNG)

#define cpPRNGen OWNAPI(cpPRNGen)
   IPP_OWN_DECL (int, cpPRNGen, (Ipp32u* pBuffer, cpSize bitLen, IppsPRNGState* pCtx))
#define cpPRNGenPattern OWNAPI(cpPRNGenPattern)
   IPP_OWN_DECL (int, cpPRNGenPattern, (BNU_CHUNK_T* pRand, int bitSize, BNU_CHUNK_T botPattern, BNU_CHUNK_T topPattern, IppBitSupplier rndFunc, void* pRndParam))
#define cpPRNGenRange OWNAPI(cpPRNGenRange)
   IPP_OWN_DECL (int, cpPRNGenRange, (BNU_CHUNK_T* pRand, const BNU_CHUNK_T* pLo, cpSize loLen, const BNU_CHUNK_T* pHi, cpSize hiLen, IppBitSupplier rndFunc, void* pRndParam))

#endif /* _CP_PRNG_H */
