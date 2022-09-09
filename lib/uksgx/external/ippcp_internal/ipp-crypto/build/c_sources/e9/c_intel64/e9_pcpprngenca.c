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
//        ippsPRNGen()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcphash.h"
#include "pcpprng.h"
#include "pcptool.h"

/*F*
// Name: ippsPRNGen
//
// Purpose: Generates a pseudorandom bit sequence of the specified nBits length.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//                               NULL == pRand
//
//    ippStsContextMatchErr      illegal pCtx->idCtx
//
//    ippStsLengthErr            1 > nBits
//
//    ippStsNoErr                no error
//
// Parameters:
//    pRand    pointer to the buffer
//    nBits    number of bits be requested
//    pCtx     pointer to the context
*F*/

IPPFUN(IppStatus, ippsPRNGen,(Ipp32u* pRand, int nBits, void* pCtx))
{
   IppsPRNGState* pCtxCtx = (IppsPRNGState*)pCtx;

   /* test PRNG context */
   IPP_BAD_PTR2_RET(pRand, pCtx);
   IPP_BADARG_RET(!RAND_VALID_ID(pCtxCtx), ippStsContextMatchErr);

   /* test sizes */
   IPP_BADARG_RET(nBits< 1, ippStsLengthErr);

   {
      cpSize rndSize = BITS2WORD32_SIZE(nBits);
      Ipp32u rndMask = MAKEMASK32(nBits);

      cpPRNGen(pRand, nBits, pCtxCtx);
      pRand[rndSize-1] &= rndMask;

      return ippStsNoErr;
   }
}
