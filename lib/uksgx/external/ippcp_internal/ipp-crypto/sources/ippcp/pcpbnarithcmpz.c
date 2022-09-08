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
//     ippsCmpZero_BN()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: ippsCmpZero_BN
//
// Purpose: Compare BigNum value with zero.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pBN == NULL
//                            pResult == NULL
//    ippStsContextMatchErr   !BN_VALID_ID(pBN)
//    ippStsNoErr             no errors
//
// Parameters:
//    pBN      BigNum ctx
//    pResult  result of comparison
//
*F*/
IPPFUN(IppStatus, ippsCmpZero_BN, (const IppsBigNumState* pBN, Ipp32u* pResult))
{
   IPP_BAD_PTR2_RET(pBN, pResult);

   IPP_BADARG_RET(!BN_VALID_ID(pBN), ippStsContextMatchErr);

   if(BN_SIZE(pBN)==1 && BN_NUMBER(pBN)[0]==0)
      *pResult = IS_ZERO;
   else if (BN_SIGN(pBN)==ippBigNumPOS)
      *pResult = GREATER_THAN_ZERO;
   else if (BN_SIGN(pBN)==ippBigNumNEG)
      *pResult = LESS_THAN_ZERO;

   return ippStsNoErr;
}
