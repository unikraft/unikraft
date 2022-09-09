/*******************************************************************************
* Copyright 2018-2021 Intel Corporation
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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Operations over GF(p).
// 
//     Context:
//        ippsGFpSetElementRegular()
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


/*F*
// Name: ippsGFpSetElement
//
// Purpose: Set GF Element from the input Big Number
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pGFp
//                               NULL == pR
//                               NULL == pBN
//
//    ippStsContextMatchErr      invalid pBN->idCtx  
//
//    ippStsOutOfRangeErr        BN is not positive
//
//    ippStsNoErr                no error
//
// Parameters:
//    pBN         pointer to the Big Number context
//    pR          pointer to Finite Field Element context
//    pGFp        pointer to Finite Field context
*F*/

IPPFUN(IppStatus, ippsGFpSetElementRegular,(const IppsBigNumState* pBN, IppsGFpElement* pR, IppsGFpState* pGFp))
{
   IPP_BAD_PTR1_RET(pBN);
   IPP_BADARG_RET( !BN_VALID_ID(pBN), ippStsContextMatchErr );
   IPP_BADARG_RET( !BN_POSITIVE(pBN), ippStsOutOfRangeErr);

   return ippsGFpSetElement((Ipp32u*)BN_NUMBER(pBN), BITS2WORD32_SIZE( BITSIZE_BNU(BN_NUMBER((pBN)),BN_SIZE((pBN)))), pR, pGFp);
}
