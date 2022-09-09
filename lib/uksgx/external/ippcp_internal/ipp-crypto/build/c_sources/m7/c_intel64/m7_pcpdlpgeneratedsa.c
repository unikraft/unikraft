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
//     DL over Prime Finite Field (generate domain parameters)
//
//  Contents:
//     ippsDLPGenerateDH()
//     ippsDLPGenerateDSA()
//
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"

/*F*
//    Name: ippsDLPGenerateDSA
//
// Purpose: Generate DL (DSA) Domain Parameters.
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pDL
//                            NULL == rndFunc
//
//    ippStsContextMatchErr   illegal pDL->idCtx
//                            illegal pSeedIn->idCtx
//                            illegal pSeedOut->idCtx
//
//    ippStsSizeErr           !(MIN_DLPDLP_BITSIZE <= DLP_BITSIZEP() <= MAX_DLPDLP_BITSIZE)
//                            !(64|DLP_BITSIZEP())
//                            !(DEF_DLPDLP_BITSIZER == DLP_BITSIZER())
//
//    ippStsRangeErr          BitSize(pSeedIn) < MIN_DLPDLP_SEEDSIZE
//                            BitSize(pSeedIn) > DLP_BITSIZEP()
//                            no room for pSeedOut
//
//    ippStsBadArgErr         nTrials <=0
//
//    ippStsInsufficientEntropy
//                            genration failure due to poor random generator
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pSeedIn     pointer to the input seed (probably null)
//    nTrials     number of trials of primality test
//    pDL         pointer to the DL context
//    pSeedOut    pointer to the output seed (probably null, or ==pSeedIn)
//    pCounter    pointer to the generator loop counter (probably null)
//    rndFunc     external random generator
//    pRndParam   pointer to the external random generator params
//
// Note:
//    1) pSeedIn==NULL means, that rndFunc will be used for input seed generation
//    2) PseedIn!=NULL limited by DSA bitsize (L) parameter!
*F*/
#define R_MAXLOOP  (100)
#define P_MAXLOOP (4096)
#define G_MAXLOOP  (100)

IPPFUN(IppStatus, ippsDLPGenerateDSA,(const IppsBigNumState* pSeedIn,
                                      int nTrials, IppsDLPState *pDL,
                                      IppsBigNumState* pSeedOut, int* pCounter,
                                      IppBitSupplier rndFunc, void* pRndParam))
{
   /* test DL context */
   IPP_BAD_PTR1_RET(pDL);
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test DL sizes */
   IPP_BADARG_RET(DEF_DLPDSA_BITSIZER != DLP_BITSIZER(pDL),ippStsSizeErr);
   IPP_BADARG_RET(MIN_DLPDSA_BITSIZE  >DLP_BITSIZEP(pDL),  ippStsSizeErr);
   IPP_BADARG_RET(MAX_DLPDSA_BITSIZE  <DLP_BITSIZEP(pDL), ippStsSizeErr);
   IPP_BADARG_RET(DLP_BITSIZEP(pDL)%64, ippStsSizeErr);

   /* test number of trials for primality check */
   IPP_BADARG_RET(nTrials<=0, ippStsBadArgErr);

   IPP_BAD_PTR1_RET(rndFunc);

   {
      /* allocate BN resources */
      BigNumNode* pList = DLP_BNCTX(pDL);
      IppsBigNumState* pP = cpBN_zero( cpBigNumListGet(&pList) );
      IppsBigNumState* pR = cpBN_zero( cpBigNumListGet(&pList) );
      IppsBigNumState* pG = cpBN_zero( cpBigNumListGet(&pList) );

      IppsBigNumState* pW = cpBN_zero( cpBigNumListGet(&pList) );
      IppsBigNumState* pX = cpBN_zero( cpBigNumListGet(&pList) );
      IppsBigNumState* pC = cpBN_zero( cpBigNumListGet(&pList) );

      IppsBigNumState* pSeed = cpBigNumListGet(&pList);

      /* interally generates SeedIn value */
      int seedBitSize = MIN_DLPDSA_SEEDSIZE;
      IppBool seed_is_random = ippTrue;

      DLP_FLAG(pDL) = 0;
      /*
      // DSA generator uses input SEED
      // either defined by user or generated internally
      */
      if(pSeedIn) {
         /* test SeedIn */
         IPP_BADARG_RET(!BN_VALID_ID(pSeedIn), ippStsContextMatchErr);
         seedBitSize = BITSIZE_BNU(BN_NUMBER(pSeedIn), BN_SIZE(pSeedIn));

         /** seedBitSize must be
         - >= MIN_DLPDSA_SEEDSIZE (==160 bits) - it's FIPS-186 claim
         - divisible by 8
           (week request, because provided automatically by BITS2WORD8_SIZE(seedBitSize) conversion)

         If seedBitSize (been calculated above) <160 this means
         that there are some zero msb bits in SeedIn representation.
         For example, if SeedIn hex string is
            is 0000000000000000000000000000000000000001
            or even 01
         we'll think about 160 bit length
         **/
         if(seedBitSize<MIN_DLPDSA_SEEDSIZE)
            seedBitSize = MIN_DLPDSA_SEEDSIZE;
         IPP_BADARG_RET(DLP_BITSIZEP(pDL)<seedBitSize, ippStsRangeErr);

         cpBN_copy(pSeed, pSeedIn);
         seed_is_random = ippFalse;
      }

      /* test SeedOut if requested */
      if(pSeedOut) {
         IPP_BADARG_RET(!BN_VALID_ID(pSeedOut), ippStsContextMatchErr);
         IPP_BADARG_RET(BITSIZE(BNU_CHUNK_T)*BN_ROOM(pSeedOut)<seedBitSize, ippStsRangeErr);
      }

      /*
      // generation DSA domain parameters
      */
      {
         int genCounter;
         int n;
         int b;
         Ipp32u result;

         IppsPrimeState* pPrimeGen = DLP_PRIMEGEN(pDL);

         /* pointer to the BNU32-value of SEED */
         Ipp32u* pSeedBNU32 = (Ipp32u*)BN_NUMBER(pSeed);
         int seedSize32 = BITS2WORD32_SIZE(seedBitSize);
         /* pointer to the octet-string-value of SEED */
         Ipp8u* pSeedOct = (Ipp8u*)BN_BUFFER(pSeed);
         int octSize;
         Ipp32u seedMask32 = MAKEMASK32(seedBitSize);

         Ipp8u shaDgst1[BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE)];
         Ipp8u shaDgst2[BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE)];

         /*
         // generate prime R,
         // R = SHA1[SEED] ^ SHA1[(SEED+1) mod 2^seedBitSize]
         */
         for(genCounter=0; genCounter<R_MAXLOOP; genCounter++) {
            int i;

            if(seed_is_random)
               rndFunc(pSeedBNU32, seedBitSize, pRndParam);

            /* save value of SEED if requested */
            if(pSeedOut)
               cpBN_copy(pSeedOut, pSeed);

            /* SHA1[SEED] */
            octSize = BITS2WORD8_SIZE(seedBitSize);
            cpToOctStr_BNU32(pSeedOct,octSize, pSeedBNU32,seedSize32);
            ippsSHA1MessageDigest(pSeedOct, octSize, shaDgst1);

            /* SEED = (SEED+1) mod 2^seedBitSize */
            cpInc_BNU32(pSeedBNU32, pSeedBNU32, seedSize32, 1);
            pSeedBNU32[seedSize32-1] &= seedMask32;

            /* SHA1[SEED] */
            //octSize = BNU_OS(pSeedOct, pSeedBNU,seedSize);
            cpToOctStr_BNU32(pSeedOct,octSize, pSeedBNU32,seedSize32);
            ippsSHA1MessageDigest(pSeedOct, octSize, shaDgst2);

            /* SHA1[] ^ SHA1[] */
            for(i=0; i<BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE); i++)
               shaDgst1[i] ^= shaDgst2[i];

            /* convert back into BNU format */
            BN_SIZE(pR) = cpFromOctStr_BNU(BN_NUMBER(pR), shaDgst1, BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE));

            /* decorate R */
            {
               BNU_CHUNK_T maskR = MASK_BNU_CHUNK(IPP_SHA1_DIGEST_BITSIZE);
               BN_NUMBER(pR)[BN_SIZE(pR)-1] &= maskR;
               SET_BIT(BN_NUMBER(pR), IPP_SHA1_DIGEST_BITSIZE-1);
               BN_NUMBER(pR)[0] |= 1;
            }

            /* perform primality test on Q */
            ippsPrimeSet_BN(pR, pPrimeGen);
            ippsPrimeTest(nTrials, &result, pPrimeGen, rndFunc, pRndParam);

            if(IS_PRIME==result)
               break;
            else
               seed_is_random = ippTrue;
         }
         if(R_MAXLOOP==genCounter)
            return ippStsInsufficientEntropy;


         n = (DLP_BITSIZEP(pDL)-1)/IPP_SHA1_DIGEST_BITSIZE;
         b = (DLP_BITSIZEP(pDL)-1)%IPP_SHA1_DIGEST_BITSIZE;
         /*
         // generate prime P
         */
         for(genCounter=0; genCounter<P_MAXLOOP; genCounter++) {
            int k;

            cpBN_zero(pW);
            /* W = SHA1[++SEED] | SHA1[++SEED] | ... */
            for(k=0; k<=n; k++) {
               cpInc_BNU32(pSeedBNU32, pSeedBNU32, seedSize32, 1);   /* ++SEED */
               pSeedBNU32[seedSize32-1] &= seedMask32;

               octSize = BITS2WORD8_SIZE(seedBitSize);
               cpToOctStr_BNU32(pSeedOct,octSize, pSeedBNU32,seedSize32);
               ippsSHA1MessageDigest(pSeedOct, octSize, shaDgst1);

               if(n!=k) { /* convert back whole digest */
                  cpFromOctStr_BNU32((Ipp32u*)BN_NUMBER(pW)+k*BITS2WORD32_SIZE(IPP_SHA1_DIGEST_BITSIZE),
                         shaDgst1, BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE));

               }
               else {     /* convert back part of one only */
                  int octLen = BITS2WORD8_SIZE(b);
                  cpFromOctStr_BNU32((Ipp32u*)BN_NUMBER(pW)+k*BITS2WORD32_SIZE(IPP_SHA1_DIGEST_BITSIZE),
                         shaDgst1+BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE)-octLen, octLen);
               }
            }

            /* W += 2^(L-1) */
            SET_BIT(BN_NUMBER(pW), DLP_BITSIZEP(pDL)-1);
            BN_SIZE(pW) = BITS_BNU_CHUNK(DLP_BITSIZEP(pDL));

            /* C = W (mod 2*R) */
            ippsAdd_BN(pR, pR, pX);
            ippsMod_BN(pW, pX, pC);

            /* P = W-(C-1) */
            ippsSub_BN(pW, pC, pW);
            ippsAdd_BN(pW, cpBN_OneRef(), pP);

            /* perform primality test on P */
            ippsPrimeSet_BN(pP, pPrimeGen);
            ippsPrimeTest(nTrials, &result, pPrimeGen, rndFunc, pRndParam);
            if(IS_PRIME==result)
               break;

         }
         if(P_MAXLOOP==genCounter)
            return ippStsInsufficientEntropy;

         /* save value of counter if requested */
         if(pCounter)
            *pCounter = genCounter;

         {
            /* set up motgomery(R) engine */
            gsModEngineInit(DLP_MONTR(pDL), (Ipp32u*)BN_NUMBER(pR), cpBN_bitsize(pR), DLP_MONT_POOL_LENGTH, gsModArithDLP());
            /* set up motgomery(P) engine */
            gsModEngineInit(DLP_MONTP0(pDL), (Ipp32u*)BN_NUMBER(pP), cpBN_bitsize(pP), DLP_MONT_POOL_LENGTH, gsModArithDLP());
         }


         /*
         // compute G
         */

         /* precompute W = (P-1)/R */
         ippsSub_BN(pP, cpBN_OneRef(), pP);
         ippsDiv_BN(pP, pR, pW, pX);

         /* precompute C = ENC(1) */
         cpMontEnc_BN(pC, cpBN_OneRef(), DLP_MONTP0(pDL));

         /* X = 2 */
         cpBN_copy(pX, cpBN_TwoRef());

         for(genCounter=0; genCounter<G_MAXLOOP; genCounter++) {
            cpMontEnc_BN(pG, pX, DLP_MONTP0(pDL));
            cpMontExpBin_BN(DLP_GENC(pDL), pG, pW, DLP_MONTP0(pDL));
            cpMontDec_BN(pG, DLP_GENC(pDL), DLP_MONTP0(pDL));
            //
            if(cpBN_cmp(DLP_GENC(pDL), pC))
               break;
            else
               ippsAdd_BN(pX, cpBN_OneRef(), pX);
         }
         if(G_MAXLOOP==genCounter)
            return ippStsInsufficientEntropy;
      }

      DLP_FLAG(pDL) = ippDLPkeyP|ippDLPkeyR|ippDLPkeyG;
      return ippStsNoErr;
   }
}
