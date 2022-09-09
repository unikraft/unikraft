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
//     Big Number Operations
// 
//  Contents:
//     ippsGetOctString_BN()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"


/*F*
//    Name: ippsGetOctString_BN
//
// Purpose: Convert BN value into the octet string.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pOctStr
//                               NULL == pBN
//
//    ippStsContextMatchErr      !BN_VALID_ID(pBN)
//
//    ippStsRangeErr             BN <0
//
//    ippStsLengthErr            strLen is enough for keep BN value
//
//    ippStsNoErr                no errors
//
// Parameters:
//    pBN         pointer to the source BN
//    pOctStr     pointer to the target octet string
//    strLen      octet string length
*F*/
IPPFUN(IppStatus, ippsGetOctString_BN,(Ipp8u* pOctStr, int strLen,
                                       const IppsBigNumState* pBN))
{
   IPP_BAD_PTR2_RET(pOctStr, pBN);

   IPP_BADARG_RET(!BN_VALID_ID(pBN), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pBN), ippStsRangeErr);
   IPP_BADARG_RET((0>strLen), ippStsLengthErr);

   return cpToOctStr_BNU(pOctStr,strLen, BN_NUMBER(pBN),BN_SIZE(pBN))? ippStsNoErr : ippStsLengthErr;
}
