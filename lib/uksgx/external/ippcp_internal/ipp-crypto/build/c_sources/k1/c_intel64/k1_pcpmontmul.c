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
//        ippsMontMul()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"
#include "pcptool.h"

/*F*
// Name: ippsMontMul
//
// Purpose: Computes Montgomery modular multiplication for positive big
//      number integers of Montgomery form. The following pseudocode
//      represents this function:
//      r <- ( a * b * R^(-1) ) mod m
//
// Returns:                Reason:
//      ippStsNoErr         Returns no error.
//      ippStsNullPtrErr    Returns an error when pointers are null.
//      ippStsBadArgErr     Returns an error when a or b is a negative integer.
//      ippStsScaleRangeErr Returns an error when a or b is more than m.
//      ippStsOutOfRangeErr Returns an error when IppsBigNumState *r is larger than
//                          IppsMontState *m.
//      ippStsContextMatchErr Returns an error when the context parameter does
//                          not match the operation.
//
// Parameters:
//      pA   Multiplicand within the range [0, m - 1].
//      pB   Multiplier within the range [0, m - 1].
//      pCtx Modulus.
//      pR   Montgomery multiplication result.
//
// Notes: The size of IppsBigNumState *r should not be less than the data
//      length of the modulus m.
*F*/
IPPFUN(IppStatus, ippsMontMul, (const IppsBigNumState* pA, const IppsBigNumState* pB, IppsMontState* pCtx, IppsBigNumState* pR))
{
   IPP_BAD_PTR4_RET(pA, pB, pCtx, pR);

   IPP_BADARG_RET(!MNT_VALID_ID(pCtx), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pA), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pB), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pR), ippStsContextMatchErr);

   IPP_BADARG_RET(BN_NEGATIVE(pA) || BN_NEGATIVE(pB), ippStsBadArgErr);
   IPP_BADARG_RET(cpCmp_BNU(BN_NUMBER(pA), BN_SIZE(pA), MOD_MODULUS( MNT_ENGINE(pCtx) ), MOD_LEN( MNT_ENGINE(pCtx) )) >= 0, ippStsScaleRangeErr);
   IPP_BADARG_RET(cpCmp_BNU(BN_NUMBER(pB), BN_SIZE(pB), MOD_MODULUS( MNT_ENGINE(pCtx) ), MOD_LEN( MNT_ENGINE(pCtx) )) >= 0, ippStsScaleRangeErr);
   IPP_BADARG_RET(BN_ROOM(pR) < MOD_LEN( MNT_ENGINE(pCtx) ), ippStsOutOfRangeErr);

   {
      const int usedPoolLen = 2;
      cpSize nsM = MOD_LEN( MNT_ENGINE(pCtx) );
      BNU_CHUNK_T* pDataR  = BN_NUMBER(pR);
      BNU_CHUNK_T* pDataA  = gsModPoolAlloc(MNT_ENGINE(pCtx), usedPoolLen);
      BNU_CHUNK_T* pDataB  = pDataA + nsM;
      //tbcd: temporary excluded: assert(NULL!=pDataA);

      ZEXPAND_COPY_BNU(pDataA, nsM, BN_NUMBER(pA), BN_SIZE(pA));
      ZEXPAND_COPY_BNU(pDataB, nsM, BN_NUMBER(pB), BN_SIZE(pB));

      MOD_METHOD( MNT_ENGINE(pCtx) )->mul(pDataR, pDataA, pDataB, MNT_ENGINE(pCtx));

      gsModPoolFree(MNT_ENGINE(pCtx), usedPoolLen);

      FIX_BNU(pDataR, nsM);
      BN_SIZE(pR) = nsM;
      BN_SIGN(pR) = ippBigNumPOS;

      return ippStsNoErr;
   }
}
