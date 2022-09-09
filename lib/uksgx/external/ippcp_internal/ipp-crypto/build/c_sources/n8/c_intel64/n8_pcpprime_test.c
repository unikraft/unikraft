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
//  Purpose:
//     Intel(R) Integrated Performance Primitives. Cryptographic Primitives (ippcp)
//     Prime Number Primitives.
// 
//  Contents:
//        cpPrimeTest()
//
*/

#include "owncp.h"
#include "pcpprimeg.h"
#include "pcpprng.h"
#include "pcptool.h"

/* Rabin-Miller test */
static int RabinMiller(int a, BNU_CHUNK_T* pZ, BNU_CHUNK_T* pR, cpSize nsR, BNU_CHUNK_T* pM, cpSize nsM, gsModEngine* pModEngine)
{
   /* modulus and it length and other parameters */
   const BNU_CHUNK_T* pModulus = MOD_MODULUS(pModEngine);
   cpSize modLen = MOD_LEN(pModEngine);

   const int usedPoolLen = 1;
   BNU_CHUNK_T* pBuffer  = 0;

   /* compute z = r^m mod prime */
   nsR = cpMontEnc_BNU_EX(pR, pR, nsR, pModEngine);
   cpMontExpBin_BNU(pZ, pR, nsR, pM, nsM, pModEngine);

   /* if z==1 => probably prime */
   if(0==cpCmp_BNU(pZ, modLen, MOD_MNT_R(pModEngine), modLen))
      return 1;

   pBuffer = gsModPoolAlloc(pModEngine, usedPoolLen);
   //tbcd: temporary excluded: assert(NULL!=pBuffer);

   /* if z==prime-1 => probably prime */
   cpSub_BNU(pBuffer, pModulus, MOD_MNT_R(pModEngine), modLen);
   if(0==cpCmp_BNU(pZ, modLen, pBuffer, modLen))
   {
      gsModPoolFree(pModEngine, usedPoolLen);
      return 1;
   }

   while(--a) {

      /* z = z^2 mod w */
      cpMontSqr_BNU(pZ, pZ, pModEngine);

      /* if z==1 => definitely composite */
      if(0==cpCmp_BNU(pZ, modLen, MOD_MNT_R(pModEngine), modLen))
      {
         gsModPoolFree(pModEngine, usedPoolLen);
         return 0;
      }

      /* if z==w-1 => probably prime */
      cpSub_BNU(pBuffer, pModulus, MOD_MNT_R(pModEngine), modLen);
      if(0==cpCmp_BNU(pZ, modLen, pBuffer, modLen))
      {
         gsModPoolFree(pModEngine, usedPoolLen);
         return 1;
      }
   }

   gsModPoolFree(pModEngine, usedPoolLen);

   /* if we are here, then we deal with composize */
   return 0;
}

/*
   returns:
    IPP_IS_PRIME     (==1) - prime value has been detected
    IPP_IS_COMPOSITE (==0) - composite value has been detected
   -1 - if internal error (ippStsNoErr != rndFunc())
*/

/*F*
// Name: cpPrimeTest
//
// Purpose: Test a number for being a probable prime.
//
// Returns:         Reason:
//        0           not prime number
//        1           prime number
//
// Parameters:
//    pPrime      prime number
//    primeLen    length of prime number
//    nTrials     parameter for the Miller-Rabin probable primality test
//    pCtx        pointer to the context
//    rndFunc     external PRNG
//    pRndParam   pointer to the external PRNG parameters
*F*/

IPP_OWN_DEFN (int, cpPrimeTest, (const BNU_CHUNK_T* pPrime, cpSize primeLen, cpSize nTrials, IppsPrimeState* pCtx, IppBitSupplier rndFunc, void* pRndParam))
{
   FIX_BNU(pPrime, primeLen);

   if( primeLen==1 && pPrime[0]==0)
      return 0;

   /* 2 is prime number */
   else if( primeLen==1 && pPrime[0]==2)
      return 1;

   /*
   // test number
   */
   else {
      cpSize primeBitsize = BITSIZE_BNU(pPrime, primeLen);
      cpSize primeLen32 = BITS2WORD32_SIZE(primeBitsize);

      /* apply easy prime test  */
      if( 0==cpMimimalPrimeTest((Ipp32u*)pPrime, primeLen32) )
         return 0;

      /* continue test */
      else {
         cpSize n, a;

         gsModEngine* pModEngine = PRIME_MONT(pCtx);
         BNU_CHUNK_T* pMdata = PRIME_TEMP1(pCtx);
         BNU_CHUNK_T* pRdata = PRIME_TEMP2(pCtx);
         BNU_CHUNK_T* pZdata = PRIME_TEMP3(pCtx);
         cpSize lenM, lenR;

         /* set up Montgomery engine (and save value being under the test) */
         gsModEngineInit(pModEngine, (Ipp32u*)pPrime, BITSIZE_BNU(pPrime, primeLen), MONT_DEFAULT_POOL_LENGTH, gsModArithMont());

         /* express w = m*2^a + 1 */
         cpDec_BNU(pMdata, pPrime, primeLen, 1);
         for(n=0,a=0; n<primeLen; n++) {
            cpSize da = cpNTZ_BNU(pMdata[n]);
            a += da;
            if(BNU_CHUNK_BITS != da)
               break;
         }

         lenM = cpLSR_BNU(pMdata, pMdata, primeLen, a);
         FIX_BNU(pMdata, lenM);

         /* run t-times Rabin-Miller Test */
         for(n=0; n<nTrials; n++) {
            /* get any random value (r) less that tested prime */
            ZEXPAND_BNU(pRdata, 0, MOD_LEN(pModEngine));
            if(ippStsNoErr != rndFunc((Ipp32u*)pRdata, primeBitsize, pRndParam))
               return -1;
            lenR = cpMod_BNU(pRdata, primeLen, MOD_MODULUS(pModEngine), primeLen);

            /* make sure r>=1 */
            if(!cpTst_BNU(pRdata, lenR))
               pRdata[0] |= 1;
            FIX_BNU(pRdata, lenR);

            /* Rabin-Miller test */
            if(0==RabinMiller(a, pZdata, pRdata,primeLen, pMdata,lenM, pModEngine))
               return 0;
         }

         return 1;
      }
   }
}
