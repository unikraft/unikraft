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
//        ippsPRNGSetH0()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpprng.h"

/*F*
// Name: ippsPRNGSetH0
//
// Purpose: Sets 160-bit parameter of G() function.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pCtx
//                               NULL == pH0
//
//    ippStsContextMatchErr      illegal pCtx->idCtx
//                               illegal pH0->idCtx
//
//    ippStsNoErr                no error
//
// Parameters:
//    pH0      pointer to the parameter used into G() function
//    pCtx     pointer to the context
*F*/
IPPFUN(IppStatus, ippsPRNGSetH0,(const IppsBigNumState* pH0, IppsPRNGState* pCtx))
{
   /* test PRNG context */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!RAND_VALID_ID(pCtx), ippStsContextMatchErr);

   /* test H0 */
   IPP_BAD_PTR1_RET(pH0);
   IPP_BADARG_RET(!BN_VALID_ID(pH0), ippStsContextMatchErr);

   {
      cpSize len = IPP_MIN(5, BN_SIZE(pH0)*((Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u))));
      ZEXPAND_BNU(RAND_T(pCtx), 0, (int)(sizeof(RAND_T(pCtx))/sizeof(BNU_CHUNK_T)));
      ZEXPAND_COPY_BNU((Ipp32u*)RAND_T(pCtx), (int)(sizeof(RAND_T(pCtx))/(sizeof(Ipp32u))),
                       (Ipp32u*)BN_NUMBER(pH0), len);
      return ippStsNoErr;
   }
}
