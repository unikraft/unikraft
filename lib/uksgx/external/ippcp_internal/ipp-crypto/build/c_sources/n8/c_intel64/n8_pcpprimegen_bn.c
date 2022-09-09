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
//        ippsPrimeGen_BN()
//
*/

#include "owncp.h"
#include "pcpprimeg.h"
#include "pcpprng.h"
#include "pcptool.h"

/*F*
// Name: ippsPrimeGen_BN
//
// Purpose: Generates a random probable prime Big number of the specified bitlength.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//                               NULL == rndFunc
//    ippStsContextMatchErr      !PRIME_VALID_ID()
//    ippStsLengthErr            1 > nBits
//    ippStsOutOfRangeErr        nBits > PRIME_MAXBITSIZE()
//    ippStsBadArgErr            1 > nTrials
//    ippStsInsufficientEntropy  when prime generation fails due
//                               to a poor choice of seed/context bitstream.
//    ippStsNoErr                no error
//
// Parameters:
//    pPrime      BigNum context
//    nBits       bitlength for the desired probable prime number
//    nTrials     parameter for the Miller-Rabin probable primality test
//    pCtx        pointer to the context
//    rndFunc     external PRNG
//    pRndParam   pointer to the external PRNG parameters
//
//
// Notes:
//    ippsPrimeGen_BN() returns ippStsInsufficientEntropy, if it
//    detects that it needs more entropy seed during its probable prime
//    generation. In this case, the user should update PRNG parameters
//    and call the primitive again.
*F*/

IPPFUN(IppStatus, ippsPrimeGen_BN, (IppsBigNumState* pPrime, int nBits,
                                   int nTrials,
                                   IppsPrimeState* pCtx,
                                   IppBitSupplier rndFunc, void* pRndParam))
{
   /* test generator context */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!PRIME_VALID_ID(pCtx), ippStsContextMatchErr);

   /* test BN context */
   IPP_BAD_PTR1_RET(pPrime);
   IPP_BADARG_RET(!BN_VALID_ID(pPrime), ippStsContextMatchErr);

   IPP_BADARG_RET(nBits<1, ippStsLengthErr);
   IPP_BADARG_RET(nBits>PRIME_MAXBITSIZE(pCtx), ippStsOutOfRangeErr);
   IPP_BADARG_RET(BN_ROOM(pPrime) < BITS_BNU_CHUNK(nBits), ippStsOutOfRangeErr);

   IPP_BADARG_RET(nTrials < 0, ippStsBadArgErr);
   IPP_BAD_PTR1_RET(rndFunc);

   {
      cpSize count;
      Ipp32u result = IPP_IS_COMPOSITE;

      BNU_CHUNK_T botPattern = 0x1;
      BNU_CHUNK_T topPattern = (BNU_CHUNK_T)1 << ((nBits-1)&(BNU_CHUNK_BITS-1));
      BNU_CHUNK_T topMask = MASK_BNU_CHUNK(nBits);

      BNU_CHUNK_T* pRand = BN_NUMBER(pPrime);
      cpSize randLen = BITS_BNU_CHUNK(nBits);

      ZEXPAND_BNU(pRand, 0, BN_ROOM(pPrime));
      BN_SIZE(pPrime) = randLen;
      BN_SIGN(pPrime) = ippBigNumPOS;

      if (nTrials < 1)
         nTrials = MR_rounds_p80(nBits);

      #define MAX_COUNT (1000)
      for(count=0; count<MAX_COUNT && result!=IPP_IS_PRIME; count++) {
         /* get trial number */
         IppStatus sts = rndFunc((Ipp32u*)pRand, nBits, pRndParam);
         if(ippStsNoErr!=sts)
            return sts;

         /* set up top and bottom bit to 1 */
         pRand[0] |= botPattern;
         pRand[randLen-1] &= topMask;
         pRand[randLen-1] |= topPattern;

         /* test trial number */
         sts = ippsPrimeTest_BN(pPrime, nTrials, &result, pCtx, rndFunc, pRndParam);
         if(ippStsNoErr!=sts)
            return sts;
      }
      #undef MAX_COUNT

      return result==IPP_IS_PRIME? ippStsNoErr : ippStsInsufficientEntropy;
   }
}
