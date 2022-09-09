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
//        ippsPRNGInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpprng.h"
#include "pcphash.h"
#include "pcptool.h"

/*F*
// Name: ippsPRNGInit
//
// Purpose: Initializes PRNG context
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//
//    ippStsLengthErr            seedBits < 1
//                               seedBits < MAX_XKEY_SIZE
//                               seedBits % 8 !=0
//
//    ippStsNoErr                no error
//
// Parameters:
//    seedBits    seed bitsize
//    pCtx        pointer to the context to be initialized
*F*/
IPPFUN(IppStatus, ippsPRNGInit, (int seedBits, IppsPRNGState* pCtx))
{
   /* test PRNG context */
   IPP_BAD_PTR1_RET(pCtx);

   /* test sizes */
   IPP_BADARG_RET((1>seedBits) || (seedBits>MAX_XKEY_SIZE) ||(seedBits&7), ippStsLengthErr);

   {
      int hashIvSize = cpHashIvSize(ippHashAlg_SHA1);
      const Ipp8u* iv = cpHashIV[ippHashAlg_SHA1];

      /* cleanup context */
      ZEXPAND_BNU((Ipp8u*)pCtx, 0, (cpSize)(sizeof(IppsPRNGState)));

      RAND_SET_ID(pCtx);
      RAND_SEEDBITS(pCtx) = seedBits;

      /* default Q parameter */
      ((Ipp32u*)RAND_Q(pCtx))[0] = 0xFFFFFFFF;
      ((Ipp32u*)RAND_Q(pCtx))[1] = 0xFFFFFFFF;
      ((Ipp32u*)RAND_Q(pCtx))[2] = 0xFFFFFFFF;
      ((Ipp32u*)RAND_Q(pCtx))[3] = 0xFFFFFFFF;
      ((Ipp32u*)RAND_Q(pCtx))[4] = 0xFFFFFFFF;

      /* default T parameter */
      CopyBlock_safe(iv, hashIvSize, RAND_T(pCtx), BITS2WORD8_SIZE(160));

      return ippStsNoErr;
   }
}
