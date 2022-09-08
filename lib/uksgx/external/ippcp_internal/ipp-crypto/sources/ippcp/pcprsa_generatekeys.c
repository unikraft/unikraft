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
//  Contents:
//        ippsRSA_GenerateKeys()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpprimeg.h"
#include "pcpprng.h"
#include "pcpngrsa.h"

#include "pcpprime_isco.h"
#include "pcpprime_isprob.h"

/*F*
// Name: ippsRSA_GenerateKeys
//
// Purpose: Generate RSA keys
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pSrcPublicExp
//                               NULL == pPublicExp
//                               NULL == pModulus
//                               NULL == pPrivateKeyType2
//                               NULL == pPrimeGen
//                               NULL == pBuffer
//                               NULL == rndFunc
//
//    ippStsContextMatchErr     !RSA_PRV_KEY2_VALID_ID(pPrivateKeyType2)
//                              !RSA_PRV_KEY1_VALID_ID(pPrivateKeyType1)
//                              !BN_VALID_ID(pSrcPublicExp)
//                              !BN_VALID_ID(pPublicExp)
//                              !BN_VALID_ID(pModulus)
//                              !PRIME_VALID_ID(pPrimeGen)
//
//    ippStsSizeErr              BN_ROOM(pPublicExp) < BN_SIZE(pSrcPublicExp)
//                               BN_ROOM(pModulus) < SIZE(factorPbitSize+factorQbitSize)
//                               PRIME_MAXBITSIZE(pPrimeGen) < factorPbitSize
//
//    ippStsOutOfRangeErr        0 >= pSrcPublicExp
//
//    ippStsBadArgErr            nTrials < 1
//
//    ippStsNoErr                no error
//
// Parameters:
//    pSrcPublicExp     pointer to the beginning public exponent
//    pPublicExp        pointer to the resulting public exponent (E)
//    pModulus          pointer to the resulting modulus (N)
//    pPrivateKeyType2  pointer to the private key type2 context
//    pPrivateKeyType1  (optional) pointer to the private key type1 context
//    pBuffer           pointer to the temporary buffer
//    nTrials           parameter of Miller-Rabin Test
//    pPrimeGen         pointer to the Prime generator context
//    rndFunc           external PRNG
//    pRndParam         pointer to the external PRNG parameters
*F*/
IPPFUN(IppStatus, ippsRSA_GenerateKeys,(const IppsBigNumState* pSrcPublicExp,
                                        IppsBigNumState* pModulus,
                                        IppsBigNumState* pPublicExp,
                                        IppsBigNumState* pPrivateExp, /* optional */
                                        IppsRSAPrivateKeyState* pPrivateKeyType2,
                                        Ipp8u* pBuffer,
                                        int nTrials,
                                        IppsPrimeState* pPrimeGen, /* NULL */
                                        IppBitSupplier rndFunc, void* pRndParam))
{
   IPP_BAD_PTR1_RET(pSrcPublicExp);
   IPP_BADARG_RET(!BN_VALID_ID(pSrcPublicExp), ippStsContextMatchErr);
   IPP_BADARG_RET(!(0 < cpBN_tst(pSrcPublicExp)), ippStsOutOfRangeErr);
   /* test if e is odd and e>=3 */
   IPP_BADARG_RET(!(BN_NUMBER(pSrcPublicExp)[0] &1), ippStsBadArgErr);
   IPP_BADARG_RET((0 > cpBN_cmp(pSrcPublicExp, cpBN_ThreeRef())), ippStsBadArgErr);

   IPP_BAD_PTR1_RET(pModulus);
   IPP_BADARG_RET(!BN_VALID_ID(pModulus), ippStsContextMatchErr);

   IPP_BAD_PTR1_RET(pPublicExp);
   IPP_BADARG_RET(!BN_VALID_ID(pPublicExp), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_ROOM(pPublicExp)<BN_SIZE(pSrcPublicExp), ippStsSizeErr);

   if(pPrivateExp) {
      IPP_BADARG_RET(!BN_VALID_ID(pPrivateExp), ippStsContextMatchErr);
   }

   IPP_BAD_PTR1_RET(pPrivateKeyType2);
   IPP_BADARG_RET(!RSA_PRV_KEY2_VALID_ID(pPrivateKeyType2), ippStsContextMatchErr);

   IPP_BAD_PTR2_RET(pBuffer, rndFunc);

   IPP_UNREFERENCED_PARAMETER(pPrimeGen);

   {
      int factorPbitSize = RSA_PRV_KEY_BITSIZE_P(pPrivateKeyType2);
      int factorQbitSize = RSA_PRV_KEY_BITSIZE_Q(pPrivateKeyType2);
      int rsaModulusBitSize = factorPbitSize + factorQbitSize;

      /* choose security parameter */
      int mrTrials = (nTrials<1)? MR_rounds_p80(factorPbitSize) : nTrials;

      /* copy input public exponen */
      cpBN_copy(pPublicExp, pSrcPublicExp);

      IPP_BADARG_RET(BN_ROOM(pModulus)<BITS_BNU_CHUNK(rsaModulusBitSize), ippStsSizeErr);
      if(pPrivateExp)
         IPP_BADARG_RET(BN_ROOM(pPrivateExp)<BITS_BNU_CHUNK(rsaModulusBitSize), ippStsSizeErr);

      {
         BNU_CHUNK_T* pFreeBuffer = (BNU_CHUNK_T*)(IPP_ALIGNED_PTR(pBuffer, (int)sizeof(BNU_CHUNK_T)));

         /* P, dP, invQ key components */
         gsModEngine* pMontP = RSA_PRV_KEY_PMONT(pPrivateKeyType2);
         BNU_CHUNK_T* pFactorP = MOD_MODULUS(pMontP);
         BNU_CHUNK_T* pExpDp = RSA_PRV_KEY_DP(pPrivateKeyType2);
         BNU_CHUNK_T* pInvQ  = RSA_PRV_KEY_INVQ(pPrivateKeyType2);
         cpSize nsP = BITS_BNU_CHUNK(factorPbitSize);
         /* Q, dQ key components */
         gsModEngine* pMontQ = RSA_PRV_KEY_QMONT(pPrivateKeyType2);
         BNU_CHUNK_T* pFactorQ = MOD_MODULUS(pMontQ);
         BNU_CHUNK_T* pExpDq = RSA_PRV_KEY_DQ(pPrivateKeyType2);
         cpSize nsQ = BITS_BNU_CHUNK(factorQbitSize);
         /* N components */
         gsModEngine* pMontN = RSA_PRV_KEY_NMONT(pPrivateKeyType2);
         BNU_CHUNK_T* pProdN = MOD_MODULUS(pMontN);
         cpSize nsN = BITS_BNU_CHUNK(factorPbitSize+factorQbitSize);

         int ret = -1;

         /*
         // generate prime P
         */
         BNU_CHUNK_T topPattern = (BNU_CHUNK_T)1 << ((factorPbitSize-1)&(BNU_CHUNK_BITS-1));
         int nRounds = 5*factorPbitSize;
         int found;
         int r;

         for(r=0,found=0; r<nRounds && !found; r++) {
            ret = cpPRNGenPattern(pFactorP, factorPbitSize, (BNU_CHUNK_T)1, topPattern, rndFunc, pRndParam);
            if(1!=ret) break; /* internal error */;

            /* chek if E and (P-1) co-prime */
            cpDec_BNU(pFactorP, pFactorP, nsP, 1);
            if(0 == cpIsCoPrime(BN_NUMBER(pPublicExp), BN_SIZE(pPublicExp), pFactorP, nsP, pFreeBuffer)) continue;

            /* test P for primality */
            cpInc_BNU(pFactorP, pFactorP, nsP, 1);
            gsModEngineInit(pMontP, (Ipp32u*)pFactorP, factorPbitSize, MOD_ENGINE_RSA_POOL_SIZE, gsModArithRSA());
          //if(0==cpIsProbablyPrime(pFactorP, factorPbitSize, mrTrials,
          //                        rndFunc, pRndParam,
          //                        pMontP, pFreeBuffer)) continue;
          //found = 1;
            ret = cpIsProbablyPrime(pFactorP, factorPbitSize, mrTrials,
                                    rndFunc, pRndParam,
                                    pMontP, pFreeBuffer);
            if(0 > ret) break;    /* internal error */
            if(0 ==ret) continue; /* composite factor */
            found = 1;
         }
         if(!found)
            goto err; /* internal error or ippStsInsufficientEntropy */

         /*
         // generate prime Q
         */
         topPattern = (BNU_CHUNK_T)1 << ((factorQbitSize-1)&(BNU_CHUNK_BITS-1));
         nRounds = 5*factorQbitSize;

         for(r=0,found=0; r<nRounds && !found; r++) {
            ret = cpPRNGenPattern(pFactorQ, factorQbitSize, (BNU_CHUNK_T)1, topPattern, rndFunc, pRndParam);
            if(1!=ret) break; /* internal error */;

            /* test if P and Q are close each other */
            if(factorPbitSize==factorQbitSize) {
               int cmp_res = cpCmp_BNU(pFactorP,nsP, pFactorQ, nsP);

               if(0==cmp_res) continue;    /* P==Q */
               else {
                  if(cmp_res<0)
                     cpSub_BNU(pFreeBuffer, pFactorQ, pFactorP, nsP);
                  else
                     cpSub_BNU(pFreeBuffer, pFactorP, pFactorQ, nsP);

                  if(factorPbitSize>=512) {
                     int bitsize = BITSIZE_BNU(pFreeBuffer, nsP);
                     if(bitsize < (factorPbitSize-100)) continue; /* abs(P-Q) <=2^(factorPbitSize-100)*/
                  }
               }
            }

            /* test if bitsize(N) = bitsize(P)+bitsize(Q) */
            cpMul_BNU_school(pProdN, pFactorP, nsP, pFactorQ, nsQ);
            if(rsaModulusBitSize != BITSIZE_BNU(pProdN, nsN)) continue;

            /* chek if E and (Q-1) co-prime */
            cpDec_BNU(pFactorQ, pFactorQ, nsQ, 1);
            if(0 == cpIsCoPrime(BN_NUMBER(pPublicExp), BN_SIZE(pPublicExp), pFactorQ, nsQ, pFreeBuffer)) continue;

            /* test Q for primality */
            cpInc_BNU(pFactorQ, pFactorQ, nsQ, 1);
            gsModEngineInit(pMontQ, (Ipp32u*)pFactorQ, factorQbitSize, MOD_ENGINE_RSA_POOL_SIZE, gsModArithRSA());
          //if(0==cpIsProbablyPrime(pFactorQ, factorQbitSize, mrTrials,
          //                        rndFunc, pRndParam,
          //                        pMontQ, pFreeBuffer)) continue;
          //found = 1;
            ret = cpIsProbablyPrime(pFactorQ, factorQbitSize, mrTrials,
                                    rndFunc, pRndParam,
                                    pMontQ, pFreeBuffer);
            if(0 > ret) break;    /* internal error */
            if(0 ==ret) continue; /* composite factor */
            found = 1;
         }
         if(!found)
            goto err; /* internal error or ippStsInsufficientEntropy */

         {
            BNU_CHUNK_T* pExpD    = pFreeBuffer;
            BNU_CHUNK_T* pExpDBuf = pExpD   +nsN+1;
            BNU_CHUNK_T* pPhi     = pExpDBuf+nsN+1;
            BNU_CHUNK_T* pPhiBuf  = pPhi    +nsN+1;
            int nsD, ns;

            /* phi = (P-1) * (Q-1) */
            cpDec_BNU(pFactorP, pFactorP, nsP, 1);
            cpDec_BNU(pFactorQ, pFactorQ, nsQ, 1);
            cpMul_BNU_school(pPhi, pFactorP, nsP, pFactorQ, nsQ);

            /* D = 1/E mod (phi) */
            nsD = cpModInv_BNU(pExpD, BN_NUMBER(pPublicExp),BN_SIZE(pPublicExp), pPhi,nsN, pExpDBuf, BN_BUFFER(pPublicExp), pPhiBuf);
            /* if D exp requested */
            if(pPrivateExp)
               BN_Set(pExpD, nsD, pPrivateExp);

            /* compute dP = D mod(P-1) */
            COPY_BNU(pExpDBuf, pExpD, nsD);
            ns = cpMod_BNU(pExpDBuf, nsD, pFactorP, nsP);
            ZEXPAND_COPY_BNU(pExpDp, nsP, pExpDBuf, ns);
            /* compute dQ = D mod(Q-1) */
            COPY_BNU(pPhi,     pExpD, nsD);
            ns = cpMod_BNU(pPhi,     nsD, pFactorQ, nsQ);
            ZEXPAND_COPY_BNU(pExpDq, nsQ, pPhi, ns);

            /* restore P and Q */
            pFactorP[0]++;
            pFactorQ[0]++;
            /* re-init Montgomery Engine */
            gsModEngineInit(pMontP, (Ipp32u*)pFactorP, factorPbitSize, MOD_ENGINE_RSA_POOL_SIZE, gsModArithRSA());
            gsModEngineInit(pMontQ, (Ipp32u*)pFactorQ, factorQbitSize, MOD_ENGINE_RSA_POOL_SIZE, gsModArithRSA());

            /* compute Qinv = 1/Q mod P */
            COPY_BNU(pPhiBuf, pFactorP,nsP);
            ns = cpModInv_BNU(pInvQ, pFactorQ,nsQ, pPhiBuf,nsP, pExpD, pExpDBuf, pPhi);
            /* expand invQ */
            ZEXPAND_BNU(pInvQ, ns, nsP);

            cpMul_BNU_school(pProdN, pFactorP, nsP, pFactorQ, nsQ);
            gsModEngineInit(pMontN, (Ipp32u*)pProdN, factorPbitSize+factorQbitSize, MOD_ENGINE_RSA_POOL_SIZE, gsModArithRSA());
            /* setup modulus */
            BN_Set(pProdN, nsN, pModulus);

            /* actual size of modulus in bits */
            RSA_PRV_KEY_BITSIZE_N(pPrivateKeyType2) = BITSIZE_BNU(pProdN, nsN);

            ret = 1;
            return ippStsNoErr;
         }

         err:
         ZEXPAND_BNU(pFactorP, 0, nsP);
         ZEXPAND_BNU(pFactorQ, 0, nsQ);
         ZEXPAND_BNU(pExpDp,   0, nsP);
         ZEXPAND_BNU(pExpDq,   0, nsQ);
         ZEXPAND_BNU(pInvQ,    0, nsP);
         return ret<0? ippStsErr : ippStsInsufficientEntropy;
      }
   }
}
