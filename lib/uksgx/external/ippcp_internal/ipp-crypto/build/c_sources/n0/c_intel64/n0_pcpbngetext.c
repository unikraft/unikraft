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
//     ippsExtGet_BN()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"

/*F*
//    Name: ippsExtGet_BN
//
// Purpose: Extracts the specified combination of the sign, data
//          length, and value characteristics of the integer big
//          number from the input structure.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pBN == NULL
//    ippStsContextMatchErr      !BN_VALID_ID(pBN)
//    ippStsNoErr                no errors
//
// Parameters:
//    pSgn     pointer to the sign
//    pBitSize pointer to the data size (in bits)
//    pData    pointer to the data buffer
//    pBN      BigNum ctx
//
*F*/

IPPFUN(IppStatus, ippsExtGet_BN, (IppsBigNumSGN* pSgn, int* pBitSize, Ipp32u* pData,
                               const IppsBigNumState* pBN))
{
   IPP_BAD_PTR1_RET(pBN);

   IPP_BADARG_RET(!BN_VALID_ID(pBN), ippStsContextMatchErr);

   {
      cpSize bitSize = BITSIZE_BNU(BN_NUMBER(pBN), BN_SIZE(pBN));
      if(0==bitSize)
         bitSize = 1;
      if(pData)
         COPY_BNU(pData, (Ipp32u*)BN_NUMBER(pBN), BITS2WORD32_SIZE(bitSize));
      if(pSgn)
         *pSgn = BN_SIGN(pBN);
      if(pBitSize)
         *pBitSize = bitSize;

      return ippStsNoErr;
   }
}
