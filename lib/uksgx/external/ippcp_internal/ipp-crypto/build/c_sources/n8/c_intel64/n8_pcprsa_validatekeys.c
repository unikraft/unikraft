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
//        ippsRSA_ValidateKeys()
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
// Name: ippsRSA_ValidateKeys
//
// Purpose: Validate RSA keys
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pPublicKey
//                               NULL == pPrivateKeyType2
//                               NULL == pPrivateKeyType1
//                               NULL == pBuffer
//                               NULL == pPrimeGen
//                               NULL == rndFunc
//                               NULL == pResult
//
//    ippStsContextMatchErr     !RSA_PUB_KEY_VALID_ID(pPublicKey)
//                              !RSA_PRV_KEY2_VALID_ID(pPrivateKeyType2)
//                              !RSA_PRV_KEY1_VALID_ID(pPrivateKeyType1)
//                              !PRIME_VALID_ID(pPrimeGen)
//
//    ippStsIncompleteContextErr public and.or private key is not set up
//
//    ippStsSizeErr              PRIME_MAXBITSIZE(pPrimeGen) < factorPbitSize
//
//    ippStsBadArgErr            nTrials < 1
//
//    ippStsNoErr                no error
//
// Parameters:
//    pResult           pointer to the validation result
//    pPublicKey        pointer to the public key context
//    pPrivateKeyType2  pointer to the private key type2 context
//    pPrivateKeyType1  (optional) pointer to the private key type1 context
//    pBuffer           pointer to the temporary buffer
//    nTrials           parameter of Miller-Rabin Test
//    pPrimeGen         pointer to the Prime generator context
//    rndFunc           external PRNG
//    pRndParam         pointer to the external PRNG parameters
*F*/
/*
// make sure D*E = 1 mod(phi(P,Q))
// where phi(P,Q) = (P-1)*(Q-1)
*/
static
int isValidPriv1_classic(const BNU_CHUNK_T* pN, int nsN,
    const BNU_CHUNK_T* pE, int nsE,
    const BNU_CHUNK_T* pD, int nsD,
    const BNU_CHUNK_T* pFactorP, int nsP,
    const BNU_CHUNK_T* pFactorQ, int nsQ,
    BNU_CHUNK_T* pBuffer)
{
    BNU_CHUNK_T* pPhi = pBuffer;
    BNU_CHUNK_T* pProduct = pPhi + nsN;
    BNU_CHUNK_T c = cpSub_BNU(pPhi, pN, pFactorP, nsP);
    int prodLen;
    if (nsN>1) cpDec_BNU(pPhi + nsP, pN + nsP, nsQ, c);
    c = cpSub_BNU(pPhi, pPhi, pFactorQ, nsQ);
    if (nsN>1) cpDec_BNU(pPhi + nsQ, pPhi + nsQ, nsP, c);
    cpInc_BNU(pPhi, pPhi, nsP + nsQ, 1);

    cpMul_BNU_school(pProduct, pE, nsE, pD, nsD);
    prodLen = cpMod_BNU(pProduct, nsE + nsD, pPhi, nsN);

    return 1 == cpEqu_BNU_CHUNK(pProduct, prodLen, 1) ? IPP_IS_VALID : IPP_IS_INVALID;
}

/*
// make sure D*E = 1 mod(lcm(P-1,Q-1))
// where lcm(P-1,Q-1) = (P-1)*(Q-1)/gcd(P-1,Q-1)
*/
static
int isValidPriv1_rsa(const BNU_CHUNK_T* pN, int nsN,
    const BNU_CHUNK_T* pE, int nsE,
    const BNU_CHUNK_T* pD, int nsD,
    BNU_CHUNK_T* pFactorP, int nsP,
    BNU_CHUNK_T* pFactorQ, int nsQ,
    BNU_CHUNK_T* pBuffer)
{
    __ALIGN8 IppsBigNumState tmpBN1;
    __ALIGN8 IppsBigNumState tmpBN2;
    __ALIGN8 IppsBigNumState tmpBN3;

    BNU_CHUNK_T* pProduct = pBuffer;
    BNU_CHUNK_T* pGcd = pProduct + (nsN + 1);
    BNU_CHUNK_T* pLcm;
    int nsLcm;
    int prodLen;
    pBuffer = pGcd + (nsP + 1) * 2;

    /* P = P-1 and Q = Q-1 */
    pFactorP[0]--;
    pFactorQ[0]--;

    /* compute product (P-1)*(Q-1) = P*Q -P -Q +1 = N -(P-1) -(Q-1) -1 */
    {
        BNU_CHUNK_T c = cpSub_BNU(pProduct, pN, pFactorP, nsP);
        if (nsN>1) cpDec_BNU(pProduct + nsP, pN + nsP, nsQ, c);
        c = cpSub_BNU(pProduct, pProduct, pFactorQ, nsQ);
        if (nsN>1) cpDec_BNU(pProduct + nsQ, pProduct + nsQ, nsP, c);
        cpDec_BNU(pProduct, pProduct, nsN, 1);
    }

    /* compute gcd(p-1, q-1) */
    BN_Make(pGcd, pGcd + nsP + 1, nsP, &tmpBN1); /* BN(gcd) */
    BN_SIZE(&tmpBN1) = nsP;
    BN_Make(pFactorP, pBuffer, nsP, &tmpBN2); /* BN(P-1) */
    BN_SIZE(&tmpBN2) = nsP;
    BN_Make(pFactorQ, pBuffer + nsP + 1, nsQ, &tmpBN3); /* BN(Q-1) */
    BN_SIZE(&tmpBN3) = nsQ;
    ippsGcd_BN(&tmpBN2, &tmpBN3, &tmpBN1);

    /* compute lcm(p-1, q-1) = (p-1)(q-1)/gcd(p-1, q-1) */
    pLcm = pBuffer;
    cpDiv_BNU(pLcm, &nsLcm, pProduct, nsN, pGcd, BN_SIZE(&tmpBN1));

    /* test E*D = 1 mod lcm */
    cpMul_BNU_school(pProduct, pE, nsE, pD, nsD);
    prodLen = cpMod_BNU(pProduct, nsE + nsD, pLcm, nsLcm);

    /* restore P and Q */
    pFactorP[0]++;
    pFactorQ[0]++;

    return 1 == cpEqu_BNU_CHUNK(pProduct, prodLen, 1) ? IPP_IS_VALID : IPP_IS_INVALID;
}

IPPFUN(IppStatus, ippsRSA_ValidateKeys,(int* pResult,
                                  const IppsRSAPublicKeyState* pPublicKey,
                                  const IppsRSAPrivateKeyState* pPrivateKeyType2,
                                  const IppsRSAPrivateKeyState* pPrivateKeyType1, /*optional */
                                        Ipp8u* pBuffer,
                                        int nTrials,
                                        IppsPrimeState* pPrimeGen, /*NULL */
                                        IppBitSupplier rndFunc, void* pRndParam))
{
   IPP_BAD_PTR1_RET(pPublicKey);
   IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(pPublicKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PUB_KEY_IS_SET(pPublicKey), ippStsIncompleteContextErr);

   IPP_BAD_PTR1_RET(pPrivateKeyType2);
   IPP_BADARG_RET(!RSA_PRV_KEY2_VALID_ID(pPrivateKeyType2), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PRV_KEY_IS_SET(pPrivateKeyType2), ippStsIncompleteContextErr);

   if(pPrivateKeyType1) { /* pPrivateKeyType1 is optional */
      IPP_BADARG_RET(!RSA_PRV_KEY1_VALID_ID(pPrivateKeyType1), ippStsContextMatchErr);
      IPP_BADARG_RET(!RSA_PRV_KEY_IS_SET(pPrivateKeyType1), ippStsIncompleteContextErr);
   }

   IPP_BAD_PTR3_RET(pResult, pBuffer, rndFunc);

   IPP_UNREFERENCED_PARAMETER(pPrimeGen);

   {
      /* E key component */
      BNU_CHUNK_T* pExpE = RSA_PUB_KEY_E(pPublicKey);
      cpSize nsE = BITS_BNU_CHUNK(RSA_PUB_KEY_BITSIZE_E(pPublicKey));

      /* N key component */
      BNU_CHUNK_T* pN = MOD_MODULUS(RSA_PUB_KEY_NMONT(pPublicKey));
      cpSize nsN = MOD_LEN(RSA_PUB_KEY_NMONT(pPublicKey));

      /* P, Q, dP, dQ, invQ key components */
      gsModEngine* pMontP = RSA_PRV_KEY_PMONT(pPrivateKeyType2);
      gsModEngine* pMontQ = RSA_PRV_KEY_QMONT(pPrivateKeyType2);
      BNU_CHUNK_T* pFactorP= MOD_MODULUS(pMontP);
      BNU_CHUNK_T* pFactorQ= MOD_MODULUS(pMontQ);
      BNU_CHUNK_T* pExpDp = RSA_PRV_KEY_DP(pPrivateKeyType2);
      BNU_CHUNK_T* pExpDq = RSA_PRV_KEY_DQ(pPrivateKeyType2);
      BNU_CHUNK_T* pInvQ  = RSA_PRV_KEY_INVQ(pPrivateKeyType2);

      int factorPbitSize = RSA_PRV_KEY_BITSIZE_P(pPrivateKeyType2);
      int factorQbitSize = RSA_PRV_KEY_BITSIZE_Q(pPrivateKeyType2);
      cpSize nsP = MOD_LEN(RSA_PRV_KEY_PMONT(pPrivateKeyType2));
      cpSize nsQ = MOD_LEN(RSA_PRV_KEY_QMONT(pPrivateKeyType2));

      int ret = IPP_IS_VALID;

      BNU_CHUNK_T* pTmp = (BNU_CHUNK_T*)(IPP_ALIGNED_PTR(pBuffer, (int)sizeof(BNU_CHUNK_T)));
      BNU_CHUNK_T* pFreeBuffer = pTmp+nsN;

      /* choose security parameter */
      int mrTrials = (nTrials<1)? MR_rounds_p80(factorPbitSize) : nTrials;

      IppStatus sts = ippStsNoErr;
      do {
         /* test if E is odd and 3 <= E < N */
         if(0==(pExpE[0]&1)) {
            ret = IPP_IS_INVALID;
            break;
         }
         if(1==nsE && pExpE[0]<3) {
            ret = IPP_IS_INVALID;
            break;
         }
         if(0 <= cpCmp_BNU(pExpE, nsE, pN, nsN)) {
            ret = IPP_IS_INVALID;
            break;
         }

         /* test if N==P*Q */
         cpMul_BNU_school(pFreeBuffer, pFactorP, nsP, pFactorQ, nsQ);
         {
            cpSize ns = cpFix_BNU(pFreeBuffer, nsP+nsQ);
            if(cpCmp_BNU(pFreeBuffer, ns, pN, nsN)) {
               ret = IPP_IS_INVALID;
               break;
            }
         }
         /* test if PubKey(N)==PrivKeytype2(N) */
         if(cpCmp_BNU(pN, nsN, MOD_MODULUS(RSA_PRV_KEY_NMONT(pPrivateKeyType2)), MOD_LEN(RSA_PRV_KEY_NMONT(pPrivateKeyType2)))) {
            ret = IPP_IS_INVALID;
            break;
         }
         /* test if PubKey(N)==PrivKeytype1(N) */
         if(pPrivateKeyType1) {
            if(cpCmp_BNU(pN, nsN, MOD_MODULUS(RSA_PRV_KEY_NMONT(pPrivateKeyType1)), MOD_LEN(RSA_PRV_KEY_NMONT(pPrivateKeyType1)))) {
               ret = IPP_IS_INVALID;
               break;
            }
         }

         /* test if P is prime */
       //if(0==cpIsProbablyPrime(pFactorP, factorPbitSize, mrTrials,
       //                        rndFunc, pRndParam,
       //                        pMontP, pFreeBuffer)) {
       //   ret = IPP_IS_COMPOSITE;
       //   break;
       //}
         {
            int r = cpIsProbablyPrime(pFactorP, factorPbitSize, mrTrials,
                                      rndFunc, pRndParam,
                                      pMontP, pFreeBuffer);
            if(0>r) {
               sts = ippStsErr;
               break;
            }
            if(0==r) {
               ret = IPP_IS_COMPOSITE;
               break;
            }
         }
         /* test if E and (P-1) co-prime */
         cpDec_BNU(pTmp, pFactorP, nsP, 1);
         if(0 == cpIsCoPrime(pExpE, nsE, pTmp, nsP, pFreeBuffer)) {
            ret = IPP_IS_INVALID;
            break;
         }
         /* test if E*dP = 1 mod (P-1) */
         cpMul_BNU_school(pFreeBuffer, pExpDp, nsP, pExpE, nsE);
         cpMod_BNU(pFreeBuffer, nsP+nsE, pTmp, nsP);
         if(!cpEqu_BNU_CHUNK(pFreeBuffer, nsP, 1)) {
            ret = IPP_IS_INVALID;
            break;
         }

         /* test if Q is prime */
       //if(0==cpIsProbablyPrime(pFactorQ, factorQbitSize, mrTrials,
       //                        rndFunc, pRndParam,
       //                        pMontQ, pFreeBuffer)) {
       //   ret = IPP_IS_COMPOSITE;
       //   break;
       //}
         {
            int r = cpIsProbablyPrime(pFactorQ, factorQbitSize, mrTrials,
                                      rndFunc, pRndParam,
                                      pMontQ, pFreeBuffer);
            if(0>r) {
               sts = ippStsErr;
               break;
            }
            if(0==r) {
               ret = IPP_IS_COMPOSITE;
               break;
            }
         }
         /* test if E and (Q-1) co-prime */
         cpDec_BNU(pTmp, pFactorQ, nsQ, 1);
         if(0 == cpIsCoPrime(pExpE, nsE, pTmp, nsQ, pFreeBuffer)) {
            ret = IPP_IS_INVALID;
            break;
         }
         /* test if E*dQ = 1 mod (Q-1) */
         cpMul_BNU_school(pFreeBuffer, pExpDq, nsQ, pExpE, nsE);
         cpMod_BNU(pFreeBuffer, nsQ+nsE, pTmp, nsQ);
         if(!cpEqu_BNU_CHUNK(pFreeBuffer, nsQ, 1)) {
            ret = IPP_IS_INVALID;
            break;
         }

         /* test if 1==(Q*Qinv) mod P */
         cpMul_BNU_school(pFreeBuffer, pInvQ, nsP, pFactorQ,nsQ);
         cpMod_BNU(pFreeBuffer, nsP+nsQ, pFactorP, nsP);
         if(!cpEqu_BNU_CHUNK(pFreeBuffer, nsP, 1)) {
            ret = IPP_IS_INVALID;
            break;
         }

         /* test private exponent (optional) */
         if(pPrivateKeyType1) {
            const BNU_CHUNK_T* pExpD = RSA_PRV_KEY_D(pPrivateKeyType1);
            cpSize nsD = nsN;
            int resilt1 = isValidPriv1_classic(pN,nsN, pExpE,nsE, pExpD,nsD,
                                               pFactorP,nsP, pFactorQ,nsQ,
                                               pTmp);
            int resilt2 = isValidPriv1_rsa(pN,nsN, pExpE,nsE, pExpD,nsD,
                                           pFactorP,nsP, pFactorQ,nsQ,
                                           pTmp);
            if(IPP_IS_VALID!=resilt1 && IPP_IS_VALID!=resilt2) {
               ret = IPP_IS_INVALID;
               break;
            }
         }
      } while(0);

      *pResult = ret;
      return sts;
   }
}
