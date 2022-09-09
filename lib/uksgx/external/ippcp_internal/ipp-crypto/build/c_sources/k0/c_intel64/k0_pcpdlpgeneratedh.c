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
//    Name: ippsDLPGenerateDH
//
// Purpose: Generate DL (DH) Domain Parameters.
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pDH
//                            NULL == rndFunc
//
//    ippStsContextMatchErr   illegal pDH->idCtx
//                            illegal pSeedIn->idCtx
//                            illegal pSeedOut->idCtx
//
//    ippStsSizeErr           !(MIN_DLPDH_BITSIZE <= DLP_BITSIZEP())
//                            !(256|DLP_BITSIZEP())
//                            !(MIN_DLPDH_BITSIZER <= DLP_BITSIZER())
//
//    ippStsRangeErr          BitSize(pSeedIn) < DH_BITSIZER()
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
//    2) PseedIn!=NULL limited by DL bitsizeP parameter!
*F*/
#define MAXLOOP (4096)

IPPFUN(IppStatus, ippsDLPGenerateDH,(const IppsBigNumState* pSeedIn,
                                     int nTrials, IppsDLPState* pDL,
                                     IppsBigNumState* pSeedOut, int* pCounter,
                                     IppBitSupplier rndFunc, void* pRndParam))
{
   /* test DL context */
   IPP_BAD_PTR1_RET(pDL);
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test DL sizes */
   IPP_BADARG_RET(MIN_DLPDH_BITSIZER>DLP_BITSIZER(pDL), ippStsSizeErr);
   IPP_BADARG_RET(MIN_DLPDH_BITSIZE >DLP_BITSIZEP(pDL), ippStsSizeErr);
   IPP_BADARG_RET(DLP_BITSIZEP(pDL)%256, ippStsSizeErr);

   /* test number of trials for primality check */
   IPP_BADARG_RET(nTrials<=0, ippStsBadArgErr);

   IPP_BAD_PTR1_RET(rndFunc);

   {
      /* allocate BN resources */
      BigNumNode* pList = DLP_BNCTX(pDL);
      IppsBigNumState* pP = cpBigNumListGet(&pList);
      IppsBigNumState* pR = cpBigNumListGet(&pList);
      IppsBigNumState* pG = cpBigNumListGet(&pList);

      IppsBigNumState* pSeed1 = cpBigNumListGet(&pList);
      IppsBigNumState* pSeed2 = cpBigNumListGet(&pList);

      /* interally generates SeedIn value by default */
      IppBool seed_is_random = ippTrue;
      int seedBitSize = DLP_BITSIZER(pDL);

      DLP_FLAG(pDL) = 0;
      /*
      // DH generator uses input SEED
      // either defined by user or generated internally
      */
      if(pSeedIn) {
         /* test SeedIn */
         IPP_BADARG_RET(!BN_VALID_ID(pSeedIn), ippStsContextMatchErr);
         seedBitSize = BITSIZE_BNU(BN_NUMBER(pSeedIn), BN_SIZE(pSeedIn));
         IPP_BADARG_RET(DLP_BITSIZER(pDL)>seedBitSize, ippStsRangeErr);
         IPP_BADARG_RET(BN_ROOM(pSeed1)<BN_SIZE(pSeedIn), ippStsRangeErr);

         cpBN_copy(pSeed1, pSeedIn);
         seed_is_random = ippFalse;
      }

      /* test SeedOut if requested */
      if(pSeedOut) {
         IPP_BADARG_RET(!BN_VALID_ID(pSeedOut), ippStsContextMatchErr);
         IPP_BADARG_RET(DLP_BITSIZER(pDL)>BITSIZE(BNU_CHUNK_T)*BN_ROOM(pSeedOut), ippStsRangeErr);
      }

      /*
      // generation DSA domain parameters
      */
      {
         int feBitsize  = DLP_BITSIZEP(pDL);
         int ordBitsize = DLP_BITSIZER(pDL);
         int m = (ordBitsize + IPP_SHA1_DIGEST_BITSIZE -1)/IPP_SHA1_DIGEST_BITSIZE;
         int l = (feBitsize  + IPP_SHA1_DIGEST_BITSIZE -1)/IPP_SHA1_DIGEST_BITSIZE;
         int n = (feBitsize  + 1023)/1024;

         IppsPrimeState* pPrimeGen = DLP_PRIMEGEN(pDL);

         /* pointers to the BNU32-value of SEED */
         Ipp32u* pSeed1BNU32 = (Ipp32u*)BN_NUMBER(pSeed1);
         Ipp32u* pSeed2BNU32 = (Ipp32u*)BN_NUMBER(pSeed2);
         int seedSize32 = BITS2WORD32_SIZE(seedBitSize);
         /* pointers to the octet-string-value of SEED */
         Ipp8u* pSeed1Oct = (Ipp8u*)BN_BUFFER(pSeed1);
         Ipp8u* pSeed2Oct = (Ipp8u*)BN_BUFFER(pSeed2);

         int octSize;
         Ipp8u shaDgst1[BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE)];
         Ipp8u shaDgst2[BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE)];

         int primeGenerated = 0;
         int genCounter;

         ippsSet_BN(ippBigNumPOS, 1, (Ipp32u*)&m, pP); /* P = m */

         /*
         // generate prime Q
         */
         for(genCounter=0; genCounter<MAXLOOP && !primeGenerated; genCounter++) {
            int i;

            if(seed_is_random) {
               rndFunc(pSeed1BNU32, seedBitSize, pRndParam);
               ippsSet_BN(ippBigNumPOS, seedSize32, pSeed1BNU32, pSeed1);
            }

            /* save value of SEED if requested */
            if(pSeedOut)
               cpBN_copy(pSeedOut, pSeed1);

            /* pSeed2 = pSeed1+m */
            ippsAdd_BN(pSeed1, pP, pSeed2);

            /* R += (SHA1[SEED+i] ^ SHA1[(SEED+m+i)) * (2^i*IPP_SHA1_DIGEST_BITSIZE) */
            cpBN_zero(pR);
            for(i=0; i<m; i++) {
               int j;
               cpSize seed1Size32, seed2Size32;

               /* SHA1[SEED+i] */
               seed1Size32 = BN_SIZE32(pSeed1);
               octSize = BITS2WORD8_SIZE( BITSIZE_BNU32(pSeed1BNU32,seed1Size32) );
               cpToOctStr_BNU32(pSeed1Oct,octSize, pSeed1BNU32, seed1Size32);
               ippsSHA1MessageDigest(pSeed1Oct, octSize, shaDgst1);

               /* SHA1[SEED+m+i] */
               seed2Size32 = BN_SIZE32(pSeed2);
               octSize = BITS2WORD8_SIZE( BITSIZE_BNU32(pSeed2BNU32,seed2Size32) );
               cpToOctStr_BNU32(pSeed2Oct,octSize, pSeed2BNU32,seedSize32);
               ippsSHA1MessageDigest(pSeed2Oct, octSize, shaDgst2);

               /* SHA1[] ^ SHA1[] */
               for(j=0; j<BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE); j++)
                  shaDgst1[j] ^= shaDgst2[j];

               if(i!=m-1) { /* convert back whole digest */
                  cpFromOctStr_BNU32((Ipp32u*)BN_NUMBER(pR)+i*BITS2WORD32_SIZE(IPP_SHA1_DIGEST_BITSIZE),
                         shaDgst1, BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE));

               }
               else {     /* convert back part of one only */
                  octSize = BITS2WORD8_SIZE( ordBitsize-IPP_SHA1_DIGEST_BITSIZE*i );
                  cpFromOctStr_BNU32((Ipp32u*)BN_NUMBER(pR)+i*BITS2WORD32_SIZE(IPP_SHA1_DIGEST_BITSIZE),
                         shaDgst1+BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE)-octSize, octSize);
               }

               /* increment both SEED instances */
               ippsAdd_BN(pSeed1, cpBN_OneRef(), pSeed1);
               ippsAdd_BN(pSeed2, cpBN_OneRef(), pSeed2);
            }

            /* decorate R */
            {
               BNU_CHUNK_T maskR = MASK_BNU_CHUNK(ordBitsize);
               BN_SIZE(pR) = BITS_BNU_CHUNK(ordBitsize);
               BN_NUMBER(pR)[BN_SIZE(pR)-1] &= maskR;
               SET_BIT(BN_NUMBER(pR), ordBitsize-1);
               BN_NUMBER(pR)[0] |= 1;
            }

            /* perform primality test on Q */
            //ippsPrimeSet_BN(pR, pPrimeGen);
            //ippsPrimeTest(nTrials, &result, pPrimeGen, rndFunc, pRndParam);
            primeGenerated = cpPrimeTest(BN_NUMBER(pR), BN_SIZE(pR), nTrials, pPrimeGen, rndFunc, pRndParam);
            if(!primeGenerated)
               seed_is_random = ippTrue;
         }
         if(MAXLOOP==genCounter)
            return ippStsInsufficientEntropy;


         ippsAdd_BN(pSeed1, pP, pSeed1); /* SEED+2*m */
         cpBN_power2(pG, feBitsize-1);     /*  2^(L-1) */

         /*
         // generate prime P
         */
         for(genCounter=0,primeGenerated=0; genCounter<MAXLOOP*n && !primeGenerated; genCounter++) {
            int i;

            /* P += (SHA1[SEED+2*m+i]) * (2^i*IPP_SHA1_DIGEST_BITSIZE) */
            cpBN_zero(pP);
            for(i=0; i<l; i++) {
               /* SHA1[SEED+2*m+i] */
               cpSize seed1Size32 = BN_SIZE32(pSeed1);
               octSize = BITS2WORD8_SIZE( BITSIZE_BNU32(pSeed1BNU32,seed1Size32) );
               cpToOctStr_BNU32(pSeed1Oct,octSize, pSeed1BNU32,seed1Size32);
               ippsSHA1MessageDigest(pSeed1Oct, octSize, shaDgst1);

               if(i!=l-1) { /* convert back whole digest */
                  cpFromOctStr_BNU32((Ipp32u*)BN_NUMBER(pP)+i*BITS2WORD32_SIZE(IPP_SHA1_DIGEST_BITSIZE),
                         shaDgst1, BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE));

               }
               else {     /* convert back part of one only */
                  octSize = BITS2WORD8_SIZE( feBitsize-IPP_SHA1_DIGEST_BITSIZE*i );
                  cpFromOctStr_BNU32((Ipp32u*)BN_NUMBER(pP)+i*BITS2WORD32_SIZE(IPP_SHA1_DIGEST_BITSIZE),
                         shaDgst1+BITS2WORD8_SIZE(IPP_SHA1_DIGEST_BITSIZE)-octSize, octSize);
               }
               /* advance SEED */
               ippsAdd_BN(pSeed1, cpBN_OneRef(), pSeed1);
            }

            /* decorate P */
            {
               BNU_CHUNK_T maskP = MASK_BNU_CHUNK(feBitsize);
               BN_SIZE(pP) = BITS_BNU_CHUNK(feBitsize);
               BN_NUMBER(pP)[BN_SIZE(pP)-1] &= maskP;
               SET_BIT(BN_NUMBER(pP), feBitsize-1);
            }

            /* P = P - (P (mod 2*R)) +1 */
            ippsAdd_BN(pR, pR, pSeed2);
            ippsMod_BN(pP, pSeed2, pSeed2);
            ippsSub_BN(pP, pSeed2, pP);
            ippsAdd_BN(pP, cpBN_OneRef(), pP);

            if( 0<cpBN_cmp(pP, pG)) { /* P ~ 2^(L-1) */
               /* perform primality test on P */
               //ippsPrimeSet_BN(pP, pPrimeGen);
               //ippsPrimeTest(nTrials, &result, pPrimeGen, rndFunc, pRndParam);
               //if(IS_PRIME==result)
               //   break;
               primeGenerated = cpPrimeTest(BN_NUMBER(pP), BN_SIZE(pP), nTrials, pPrimeGen, rndFunc, pRndParam);
               if(primeGenerated)
                  break;
            }
         }
         if(n*MAXLOOP<=genCounter)
            return ippStsInsufficientEntropy;

         /* save value of counter if requested */
         if(pCounter)
            *pCounter = genCounter;


         /* set up Q */
         ZEXPAND_COPY_BNU(DLP_R(pDL),BITS_BNU_CHUNK(DLP_BITSIZER(pDL)), BN_NUMBER(pR), BN_SIZE(pR));

         /* set up motgomery(P) engine */
         gsModEngineInit(DLP_MONTP0(pDL), (Ipp32u*)BN_NUMBER(pP), feBitsize, DLP_MONT_POOL_LENGTH, gsModArithDLP());

         /* precompute cofactor j = (P-1)/R */
         ippsSub_BN(pP, cpBN_OneRef(), pSeed2);
         ippsDiv_BN(pSeed2, pR, pSeed1, pG);

         /* precompute ENC(1) */
         cpMontEnc_BN(pR, cpBN_OneRef(), DLP_MONTP0(pDL));

         /*
         // compute G
         */
         BN_SIGN(pG) = ippBigNumPOS;
         for(genCounter=0; genCounter<MAXLOOP; genCounter++) {
            /* random < 2^L */
            cpSize sizeG = BITS_BNU_CHUNK(feBitsize);
            rndFunc((Ipp32u*)BN_NUMBER(pG), feBitsize, pRndParam);
            FIX_BNU(BN_NUMBER(pG), sizeG);
            BN_SIZE(pG) = sizeG;
            /* G < (P-1) */
            ippsMod_BN(pG, pP, pG);

            if( !cpEqu_BNU_CHUNK(BN_NUMBER(pG), BN_SIZE(pG), 0) ) {
               cpMontEnc_BN(pG, pG, DLP_MONTP0(pDL));
               cpMontExpBin_BN(pG, pG, pSeed1, DLP_MONTP0(pDL));
               if( cpBN_cmp(pG, pR) )
                  break;
            }
         }
         if(MAXLOOP==genCounter)
            return ippStsInsufficientEntropy;

         /* set up encoded G */
         cpBN_copy(DLP_GENC(pDL), pG);
      }

      DLP_FLAG(pDL) = ippDLPkeyP|ippDLPkeyR|ippDLPkeyG;
      return ippStsNoErr;
   }
}
