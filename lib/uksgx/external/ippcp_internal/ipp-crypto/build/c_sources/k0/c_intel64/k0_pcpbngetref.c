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
//     ippsBigNumGetSize()
//     ippsBigNumInit()
// 
//     ippsSet_BN()
//     ippsGet_BN()
//     ippsGetSize_BN()
//     ippsExtGet_BN()
//     ippsRef_BN()
// 
//     ippsCmpZero_BN()
//     ippsCmp_BN()
// 
//     ippsAdd_BN()
//     ippsSub_BN()
//     ippsMul_BN()
//     ippsMAC_BN_I()
//     ippsDiv_BN()
//     ippsMod_BN()
//     ippsGcd_BN()
//     ippsModInv_BN()
// 
//     cpPackBigNumCtx(), cpUnpackBigNumCtx()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: ippsRef_BN
//
// Purpose: Get BigNum info.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pBN == NULL
//    ippStsContextMatchErr      !BN_VALID_ID(pBN)
//    ippStsNoErr                no errors
//
// Parameters:
//    pSgn     pointer to the sign
//    pBitSize pointer to the data size (in bits)
//    ppData   pointer to the data buffer
//    pBN      BigNum ctx
//
*F*/
IPPFUN(IppStatus, ippsRef_BN, (IppsBigNumSGN* pSgn, int* pBitSize, Ipp32u** const ppData,
                               const IppsBigNumState *pBN))
{
   IPP_BAD_PTR1_RET(pBN);

   IPP_BADARG_RET(!BN_VALID_ID(pBN), ippStsContextMatchErr);

   if(pSgn)
      *pSgn = BN_SIGN(pBN);
   if(pBitSize) {
      cpSize bitLen = BITSIZE_BNU(BN_NUMBER(pBN), BN_SIZE(pBN));
      *pBitSize = bitLen? bitLen : 1;
   }

   if(ppData)
      *ppData = (Ipp32u*)BN_NUMBER(pBN);

   return ippStsNoErr;
}
