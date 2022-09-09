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
//        ippsPRNGSetModulus()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpprng.h"

/*F*
// Name: ippsPRNGSetModulus
//
// Purpose: Sets 160-bit modulus Q.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//                               NULL == pMod
//
//    ippStsContextMatchErr      illegal pCtx->idCtx
//                               illegal pMod->idCtx
//
//    ippStsBadArgErr            160 != bitsize(pMod)
//
//    ippStsNoErr                no error
//
// Parameters:
//    pMod     pointer to the 160-bit modulus
//    pCtx     pointer to the context
*F*/
IPPFUN(IppStatus, ippsPRNGSetModulus, (const IppsBigNumState* pMod, IppsPRNGState* pCtx))
{
   /* test PRNG context */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!RAND_VALID_ID(pCtx), ippStsContextMatchErr);

   /* test modulus */
   IPP_BAD_PTR1_RET(pMod);
   IPP_BADARG_RET(!BN_VALID_ID(pMod), ippStsContextMatchErr);
   IPP_BADARG_RET(160 != BITSIZE_BNU(BN_NUMBER(pMod),BN_SIZE(pMod)), ippStsBadArgErr);

   ZEXPAND_COPY_BNU(RAND_Q(pCtx), (int)(sizeof(RAND_Q(pCtx))/sizeof(BNU_CHUNK_T)), BN_NUMBER(pMod),  BN_SIZE(pMod));
   return ippStsNoErr;
}
