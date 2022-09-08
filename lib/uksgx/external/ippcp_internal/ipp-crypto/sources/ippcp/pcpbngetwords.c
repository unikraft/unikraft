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
//     ippsGet_BN()
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: ippsGet_BN
//
// Purpose: Get BigNum.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pBN == NULL
//                               pData == NULL
//                               pSgn == NULL
//                               pLengthInBits ==NULL
//    ippStsContextMatchErr      !BN_VALID_ID(pBN)
//    ippStsNoErr                no errors
//
// Parameters:
//    pSgn            pointer to the sign
//    pLengthInBits   pointer to the data size (in Ipp32u chunks)
//    pData           pointer to the data buffer
//    pBN             BigNum ctx
//
*F*/
IPPFUN(IppStatus, ippsGet_BN, (IppsBigNumSGN* pSgn, int* pLengthInBits, Ipp32u* pData,
                               const IppsBigNumState* pBN))
{
   IPP_BAD_PTR4_RET(pSgn, pLengthInBits, pData, pBN);

   IPP_BADARG_RET(!BN_VALID_ID(pBN), ippStsContextMatchErr);

   {
      cpSize len32 = BN_SIZE(pBN)*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u));
      Ipp32u* bnData = (Ipp32u*)BN_NUMBER(pBN);

      FIX_BNU32(bnData, len32);
      COPY_BNU(pData, bnData, len32);

      *pSgn = BN_SIGN(pBN);
      *pLengthInBits = len32;

      return ippStsNoErr;
   }
}
