/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     RSA Functions
//
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpprimeg.h"
#include "pcpprng.h"
#include "pcpngrsa.h"


static int cpMillerRabinTest(BNU_CHUNK_T* pW, cpSize nsW,
    const BNU_CHUNK_T* pE, cpSize bitsizeE,
    int k,
    const BNU_CHUNK_T* pPrime1,
    gsModEngine* pMont,
    BNU_CHUNK_T* pBuffer)
{
    cpSize nsP = MOD_LEN(pMont);

    /* to Montgomery Domain */
    ZEXPAND_BNU(pW, nsW, nsP);
    MOD_METHOD(pMont)->encode(pW, pW, pMont);

    /* w = exp(w,e) */
    #if !defined(_USE_WINDOW_EXP_)
    gsMontExpBin_BNU_sscm(pW, pW, nsP, pE, bitsizeE, pMont, pBuffer);
    #else
    gsMontExpWin_BNU_sscm(pW, pW, nsP, pE, bitsizeE, pMont, pBuffer);
    #endif

    /* if (w==1) ||(w==prime-1) => probably prime */
    if ((0 == cpCmp_BNU(pW, nsP, MOD_MNT_R(pMont), nsP))
        || (0 == cpCmp_BNU(pW, nsP, pPrime1, nsP)))
        return 1;      /* witness of the primality */

    while (--k) {
        MOD_METHOD(pMont)->sqr(pW, pW, pMont);

        if (0 == cpCmp_BNU(pW, nsP, MOD_MNT_R(pMont), nsP))
            return 0;   /* witness of the compositeness */
        if (0 == cpCmp_BNU(pW, nsP, pPrime1, nsP))
            return 1;   /* witness of the primality */
    }
    return 0;
}

/* test if P is prime

returns:
IPP_IS_PRIME     (==1) - prime value has been detected
IPP_IS_COMPOSITE (==0) - composite value has been detected
-1 - if internal error (ippStsNoErr != rndFunc())
*/
static int cpIsProbablyPrime(BNU_CHUNK_T* pPrime, int bitSize,
    int nTrials,
    IppBitSupplier rndFunc, void* pRndParam,
    gsModEngine* pME,
    BNU_CHUNK_T* pBuffer)
{
    /* if test for trivial divisors passed*/
    int ret = cpMimimalPrimeTest((Ipp32u*)pPrime, BITS2WORD32_SIZE(bitSize));

    /* appy Miller-Rabin test */
    if (ret) {
        int ns = BITS_BNU_CHUNK(bitSize);
        BNU_CHUNK_T* pPrime1 = pBuffer;
        BNU_CHUNK_T* pOdd = pPrime1 + ns;
        BNU_CHUNK_T* pWitness = pOdd + ns;
        BNU_CHUNK_T* pMontPrime1 = pWitness + ns;
        BNU_CHUNK_T* pScratchBuffer = pMontPrime1 + ns;
        int k, a, lenOdd;

        /* prime1 = prime-1 = odd*2^a */
        cpDec_BNU(pPrime1, pPrime, ns, 1);
        for (k = 0, a = 0; k<ns; k++) {
            cpSize da = cpNTZ_BNU(pPrime1[k]);
            a += da;
            if (BNU_CHUNK_BITS != da)
                break;
        }
        lenOdd = cpLSR_BNU(pOdd, pPrime1, ns, a);
        FIX_BNU(pOdd, lenOdd);

        /* prime1 to (Montgomery Domain) */
        cpSub_BNU(pMontPrime1, pPrime, MOD_MNT_R(pME), ns);

      //for (k = 0, ret = 0; k<nTrials && !ret; k++) {
      //    BNU_CHUNK_T one = 1;
      //    ret = cpPRNGenRange(pWitness, &one, 1, pPrime1, ns, rndFunc, pRndParam);
      //    if (ret <= 0) break; /* internal error */
      //                         /* test primality */
      //    ret = cpMillerRabinTest(pWitness, ns,
      //        //pOdd, lenOdd, a,
      //        pOdd, bitSize - a, a,
      //        pMontPrime1,
      //        pME, pScratchBuffer);
      //}
       for(k=0, ret=1; k<nTrials; k++) {
            BNU_CHUNK_T one = 1;
            ret = cpPRNGenRange(pWitness, &one, 1, pPrime1, ns, rndFunc, pRndParam);
            if (ret <= 0) break; /* internal error */

            /* Millar-Rabin primality test */
            ret = cpMillerRabinTest(pWitness, ns,
                //pOdd, lenOdd, a,
                pOdd, bitSize - a, a,
                pMontPrime1,
                pME, pScratchBuffer);
            if (ret == 0) break; /* composite */
        }
    }
    return ret;
}