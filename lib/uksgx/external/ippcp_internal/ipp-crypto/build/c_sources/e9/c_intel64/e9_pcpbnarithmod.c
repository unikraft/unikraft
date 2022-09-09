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
//     ippsMod_BN()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: ippsMod_BN
//
// Purpose: reduction BigNum.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pA  == NULL
//                            pM  == NULL
//                            pR  == NULL
//    ippStsContextMatchErr   !BN_VALID_ID(pA)
//                            !BN_VALID_ID(pM)
//                            !BN_VALID_ID(pR)
//    ippStsOutOfRangeErr     pR can not hold result
//    ippStsBadModulusErr     modulus IppsBigNumState* pM
//                             is not a positive integer
//    ippStsNoErr             no errors
//
// Parameters:
//    pA    source BigNum
//    pB    source BigNum
//    pR    reminder BigNum
//
//    A = Q*M + R, 0 <= R < B
//
*F*/
IPPFUN(IppStatus, ippsMod_BN, (IppsBigNumState* pA, IppsBigNumState* pM, IppsBigNumState* pR))
{
   IPP_BAD_PTR3_RET(pA, pM, pR);

   IPP_BADARG_RET(!BN_VALID_ID(pA), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pM), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pR), ippStsContextMatchErr);

   IPP_BADARG_RET(BN_NEGATIVE(pM), ippStsBadModulusErr);
   IPP_BADARG_RET(BN_SIZE(pM)== 1 && BN_NUMBER(pM)[0]==0, ippStsBadModulusErr);

   IPP_BADARG_RET(BN_ROOM(pR)<BN_SIZE(pM), ippStsOutOfRangeErr);

   if(cpEqu_BNU_CHUNK(BN_NUMBER(pA), BN_SIZE(pA), 0)) {
      BN_SIGN(pR) = ippBigNumPOS;
      BN_SIZE(pR) = 1;
      BN_NUMBER(pR)[0] = 0;
   }

   else {
      BNU_CHUNK_T* pDataM = BN_NUMBER(pM);
      cpSize nsM = BN_SIZE(pM);
      BNU_CHUNK_T* pBuffA = BN_BUFFER(pA);
      cpSize nsA = BN_SIZE(pA);
      BNU_CHUNK_T* pDataR = BN_NUMBER(pR);
      cpSize nsR;

      COPY_BNU(pBuffA, BN_NUMBER(pA), nsA);
      nsR = cpMod_BNU(pBuffA, nsA, pDataM, nsM);

      COPY_BNU(pDataR, pBuffA, nsR);
      BN_SIZE(pR) = nsR;
      BN_SIGN(pR) = ippBigNumPOS;

      if(BN_NEGATIVE(pA) && !(nsR==1 && pDataR[0]==0)) {
         ZEXPAND_BNU(pDataR, nsR, nsM);
         cpSub_BNU(pDataR, pDataM, pDataR, nsM);
         FIX_BNU(pDataR, nsM);
         BN_SIZE(pR) = nsM;
      }
   }

   return ippStsNoErr;
}
