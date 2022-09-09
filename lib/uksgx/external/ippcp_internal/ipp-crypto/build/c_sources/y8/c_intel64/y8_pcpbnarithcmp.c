/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
//               Intel(R) Integrated Performance Primitives
//                   Cryptographic Primitives (ippcp)
// 
//  Contents:
//     ippsCmp_BN()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"

/*F*
//    Name: ippsCmp_BN
//
// Purpose: Compare two BigNums.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pA == NULL
//                            pB == NULL
//                            pResult == NULL
//    ippStsContextMatchErr   !BN_VALID_ID(pA)
//                            !BN_VALID_ID(pB)
//    ippStsNoErr             no errors
//
// Parameters:
//    pA       BigNum ctx
//    pB       BigNum ctx
//    pResult  result of comparison
//
*F*/
IPPFUN(IppStatus, ippsCmp_BN,(const IppsBigNumState* pA, const IppsBigNumState* pB, Ipp32u *pResult))
{
   IPP_BAD_PTR3_RET(pA, pB, pResult);

   IPP_BADARG_RET(!BN_VALID_ID(pA), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pB), ippStsContextMatchErr);

   {
      BNU_CHUNK_T positiveA = cpIsEqu_ct(ippBigNumPOS, BN_SIGN(pA));
      BNU_CHUNK_T positiveB = cpIsEqu_ct(ippBigNumPOS, BN_SIGN(pB));
      BNU_CHUNK_T signMask;

      /* (ippBigNumPOS == BN_SIGN(pA)) && (ippBigNumPOS==BN_SIGN(pB))  => res = cpCmp_BNU() */
      BNU_CHUNK_T res  = (BNU_CHUNK_T)( cpCmp_BNU(BN_NUMBER(pA), BN_SIZE(pA), BN_NUMBER(pB), BN_SIZE(pB)) );

      /* (ippBigNumNEG == BN_SIGN(pA)) && (ippBigNumNEG==BN_SIGN(pB))  => invert res value */
      signMask = ~positiveA & ~positiveB;
      res = (res & ~signMask) | ((0-res) & signMask);

      /* (ippBigNumPOS == BN_SIGN(pA)) && (ippBigNumNEG==BN_SIGN(pB))  => res = 1 */
      signMask = positiveA & ~positiveB;
      res = (res & ~signMask) | ((1) & signMask);

      /* (ippBigNumNEG == BN_SIGN(pA)) && (ippBigNumPOS==BN_SIGN(pB))  => res = -1 */
      signMask = ~positiveA & positiveB;
      res = (res & ~signMask) | ((BNU_CHUNK_T)(-1) & signMask);

      // map res into IPP_IS_LT/EQ/GT
      Ipp32u cmpResult = (Ipp32u)( (cpIsEqu_ct(res, (BNU_CHUNK_T)(-1)) & IPP_IS_LT)
                                 | (cpIsEqu_ct(res, (BNU_CHUNK_T)(0))  & IPP_IS_EQ)
                                 | (cpIsEqu_ct(res, (BNU_CHUNK_T)(1))  & IPP_IS_GT) );
      *pResult = cmpResult;

      return ippStsNoErr;
   }
}
