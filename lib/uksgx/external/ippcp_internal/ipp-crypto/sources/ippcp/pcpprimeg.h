/*******************************************************************************
* Copyright 2004-2021 Intel Corporation
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
//               Intel(R) Integrated Performance Primitives
//                   Cryptographic Primitives (ippcp)
//                    Prime Number Primitives.
// 
// 
*/


#if !defined(_CP_PRIME_H)
#define _CP_PRIME_H

#include "pcpbn.h"
#include "pcpmontgomery.h"


/*
// Prime context
*/
struct _cpPrime {
   Ipp32u            idCtx;      /* Prime context identifier */
   cpSize            maxBitSize; /* max bit length             */
   BNU_CHUNK_T*      pPrime;     /* prime value   */
   BNU_CHUNK_T*      pT1;        /* temporary BNU */
   BNU_CHUNK_T*      pT2;        /* temporary BNU */
   BNU_CHUNK_T*      pT3;        /* temporary BNU */
   gsModEngine*      pMont;      /* montgomery engine        */
};

/* alignment */
#define PRIME_ALIGNMENT ((int)sizeof(void*))

/* Prime accessory macros */
#define PRIME_SET_ID(ctx)     ((ctx)->idCtx = (Ipp32u)idCtxPrimeNumber ^ (Ipp32u)IPP_UINT_PTR(ctx))
#define PRIME_MAXBITSIZE(ctx) ((ctx)->maxBitSize)
#define PRIME_NUMBER(ctx)     ((ctx)->pPrime)
#define PRIME_TEMP1(ctx)      ((ctx)->pT1)
#define PRIME_TEMP2(ctx)      ((ctx)->pT2)
#define PRIME_TEMP3(ctx)      ((ctx)->pT3)
#define PRIME_MONT(ctx)       ((ctx)->pMont)

#define PRIME_VALID_ID(ctx)   ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((ctx))) == (Ipp32u)idCtxPrimeNumber)

/*
// Number of Miller-Rabin rounds for an error rate of less than 1/2^80 for random 'b'-bit input, b >= 100.
// (see Table 4.4, Handbook of Applied Cryptography [Menezes, van Oorschot, Vanstone; CRC Press 1996]
*/
#define MR_rounds_p80(b) ((b) >= 1300 ?  2 : \
                          (b) >=  850 ?  3 : \
                          (b) >=  650 ?  4 : \
                          (b) >=  550 ?  5 : \
                          (b) >=  450 ?  6 : \
                          (b) >=  400 ?  7 : \
                          (b) >=  350 ?  8 : \
                          (b) >=  300 ?  9 : \
                          (b) >=  250 ? 12 : \
                          (b) >=  200 ? 15 : \
                          (b) >=  150 ? 18 : \
                        /*(b) >=  100*/ 27)

/* easy prime test */
#define cpMimimalPrimeTest OWNAPI(cpMimimalPrimeTest)
   IPP_OWN_DECL (int, cpMimimalPrimeTest, (const Ipp32u* pPrime, cpSize ns))

/* prime test */
#define cpPrimeTest OWNAPI(cpPrimeTest)
   IPP_OWN_DECL (int, cpPrimeTest, (const BNU_CHUNK_T* pPrime, cpSize primeLen, cpSize nTrials, IppsPrimeState* pCtx, IppBitSupplier rndFunc, void* pRndParam))

#define cpPackPrimeCtx OWNAPI(cpPackPrimeCtx)
   IPP_OWN_DECL (void, cpPackPrimeCtx, (const IppsPrimeState* pCtx, Ipp8u* pBuffer))
#define cpUnpackPrimeCtx OWNAPI(cpUnpackPrimeCtx)
   IPP_OWN_DECL (void, cpUnpackPrimeCtx, (const Ipp8u* pBuffer, IppsPrimeState* pCtx))

#endif /* _CP_PRIME_H */
