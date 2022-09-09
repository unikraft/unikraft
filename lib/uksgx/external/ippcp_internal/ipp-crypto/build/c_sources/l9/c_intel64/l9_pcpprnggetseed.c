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
//        ippsPRNGGetSeed()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpprng.h"

/*F*
// Name: ippsPRNGGetSeed
//
// Purpose: Get current SEED value from the state
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//                               NULL == pSeed
//
//    ippStsContextMatchErr      illegal pCtx->idCtx
//                               illegal pSeed->idCtx
//    ippStsOutOfRangeErr        length of the actual SEED > length SEED destination
//
//    ippStsNoErr                no error
//
// Parameters:
//    pCtx     pointer to the context
//    pSeed    pointer to the SEED
*F*/
IPPFUN(IppStatus, ippsPRNGGetSeed, (const IppsPRNGState* pCtx, IppsBigNumState* pSeed))
{
   /* test PRNG context */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!RAND_VALID_ID(pCtx), ippStsContextMatchErr);

   /* test seed */
   IPP_BAD_PTR1_RET(pSeed);
   IPP_BADARG_RET(!BN_VALID_ID(pSeed), ippStsContextMatchErr);

   return ippsSet_BN(ippBigNumPOS,
                     BITS2WORD32_SIZE(RAND_SEEDBITS(pCtx)),
                     (Ipp32u*)RAND_XKEY(pCtx),
                     pSeed);
}
