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
//        ippsPRNGSetAugment()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpprng.h"

/*F*
// Name: ippsPRNGSetAugment
//
// Purpose: Sets the Entropy Augmentation
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//                               NULL == pAug
//
//    ippStsContextMatchErr      illegal pCtx->idCtx
//                               illegal pAug->idCtx
//
//    ippStsNoErr                no error
//
// Parameters:
//    pAug  pointer to the entropy eugmentation
//    pCtx  pointer to the context
*F*/

IPPFUN(IppStatus, ippsPRNGSetAugment, (const IppsBigNumState* pAug, IppsPRNGState* pCtx))
{
   /* test PRNG context */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!RAND_VALID_ID(pCtx), ippStsContextMatchErr);

   /* test augmentation */
   IPP_BAD_PTR1_RET(pAug);
   IPP_BADARG_RET(!BN_VALID_ID(pAug), ippStsContextMatchErr);

   {
      cpSize argSize = BITS_BNU_CHUNK( RAND_SEEDBITS(pCtx) );
      BNU_CHUNK_T mask = MASK_BNU_CHUNK(RAND_SEEDBITS(pCtx));
      cpSize size = IPP_MIN(BN_SIZE(pAug), argSize);

      ZEXPAND_COPY_BNU(RAND_XAUGMENT(pCtx), (cpSize)(sizeof(RAND_XAUGMENT(pCtx))/sizeof(BNU_CHUNK_T)), BN_NUMBER(pAug), size);
      RAND_XAUGMENT(pCtx)[argSize-1] &= mask;

      return ippStsNoErr;
   }
}
