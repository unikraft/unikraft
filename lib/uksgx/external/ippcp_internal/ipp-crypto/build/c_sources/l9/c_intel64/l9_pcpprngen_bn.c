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
// 
//  Purpose:
//     Cryptography Primitive.
//     PRNG Functions
// 
//  Contents:
//        ippsPRNGen_BN()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcphash.h"
#include "pcpprng.h"
#include "pcptool.h"

/*F*
// Name: ippsPRNGen_BN
//
// Purpose: Generates a pseudorandom big number of the specified nBits length.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//                               NULL == pRand
//
//    ippStsContextMatchErr      illegal pCtx->idCtx
//                               illegal pRand->idCtx
//
//    ippStsLengthErr            1 > nBits
//                               nBits > BN_ROOM(pRand)
//
//    ippStsNoErr                no error
//
// Parameters:
//    pRand    pointer to the BN random
//    nBits    number of bits be requested
//    pCtx     pointer to the context
*F*/
IPPFUN(IppStatus, ippsPRNGen_BN,(IppsBigNumState* pRand, int nBits, void* pCtx))
{
   IppsPRNGState* pRndCtx;

   /* test PRNG context */
   IPP_BAD_PTR1_RET(pCtx);
   pRndCtx = (IppsPRNGState*)(pCtx);
   IPP_BADARG_RET(!RAND_VALID_ID(pRndCtx), ippStsContextMatchErr);

   /* test random BN */
   IPP_BAD_PTR1_RET(pRand);
   IPP_BADARG_RET(!BN_VALID_ID(pRand), ippStsContextMatchErr);

   /* test sizes */
   IPP_BADARG_RET(nBits< 1, ippStsLengthErr);
   IPP_BADARG_RET(nBits> BN_ROOM(pRand)*BNU_CHUNK_BITS, ippStsLengthErr);


   {
      BNU_CHUNK_T* pRandBN = BN_NUMBER(pRand);
      cpSize rndSize = BITS_BNU_CHUNK(nBits);
      BNU_CHUNK_T rndMask = MASK_BNU_CHUNK(nBits);

      cpPRNGen((Ipp32u*)pRandBN, nBits, pRndCtx);
      pRandBN[rndSize-1] &= rndMask;

      FIX_BNU(pRandBN, rndSize);
      BN_SIZE(pRand) = rndSize;
      BN_SIGN(pRand) = ippBigNumPOS;

      return ippStsNoErr;
   }
}
