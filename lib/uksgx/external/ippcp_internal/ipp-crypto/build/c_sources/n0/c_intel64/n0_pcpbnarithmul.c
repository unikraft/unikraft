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
//     ippsMul_BN()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: ippsMul_BN
//
// Purpose: Multiply BigNums.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pA  == NULL
//                            pB  == NULL
//                            pR  == NULL
//    ippStsContextMatchErr   !BN_VALID_ID(pA)
//                            !BN_VALID_ID(pB)
//                            !BN_VALID_ID(pR)
//    ippStsOutOfRangeErr     pR can not hold result
//    ippStsNoErr             no errors
//
// Parameters:
//    pA    source BigNum
//    pB    source BigNum
//    pR    resultant BigNum
//
*F*/
IPPFUN(IppStatus, ippsMul_BN, (IppsBigNumState* pA, IppsBigNumState* pB, IppsBigNumState* pR))
{
   IPP_BAD_PTR3_RET(pA, pB, pR);

   IPP_BADARG_RET(!BN_VALID_ID(pA), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pB), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pR), ippStsContextMatchErr);

   {
      BNU_CHUNK_T* pDataA = BN_NUMBER(pA);
      BNU_CHUNK_T* pDataB = BN_NUMBER(pB);
      BNU_CHUNK_T* pDataR = BN_NUMBER(pR);

      cpSize nsA = BN_SIZE(pA);
      cpSize nsB = BN_SIZE(pB);
      cpSize nsR = BN_ROOM(pR);

      cpSize bitSizeA = BITSIZE_BNU(pDataA, nsA);
      cpSize bitSizeB = BITSIZE_BNU(pDataB, nsB);

      /* test if multiplicant/multiplier is zero */
      if(!bitSizeA || !bitSizeB) {
         BN_SIZE(pR) = 1;
         BN_SIGN(pR) = IppsBigNumPOS;
         pDataR[0] = 0;
         return ippStsNoErr;
      }

      /* test if even low estimation of product A*B exceeded */
      IPP_BADARG_RET(nsR*BNU_CHUNK_BITS < (bitSizeA+bitSizeB-1), ippStsOutOfRangeErr);

      {
         BNU_CHUNK_T* aData = pDataA;
         BNU_CHUNK_T* bData = pDataB;

         if(pA == pR) {
            aData = BN_BUFFER(pR);
            COPY_BNU(aData, pDataA, nsA);
         }
         if((pB == pR) && (pA != pB)) {
            bData = BN_BUFFER(pR);
            COPY_BNU(bData, pDataB, nsB);
         }

         /* clear result */
         ZEXPAND_BNU(pDataR, 0, nsR+1);

         if(pA==pB)
            cpSqr_BNU_school(pDataR, aData, nsA);
         else
            cpMul_BNU_school(pDataR, aData, nsA, bData, nsB);

         nsR = (bitSizeA + bitSizeB + BNU_CHUNK_BITS - 1) /BNU_CHUNK_BITS;
         FIX_BNU(pDataR, nsR);
         IPP_BADARG_RET(nsR>BN_ROOM(pR), ippStsOutOfRangeErr);

         BN_SIZE(pR) = nsR;
         BN_SIGN(pR) = (BN_SIGN(pA)==BN_SIGN(pB)? ippBigNumPOS : ippBigNumNEG);
         return ippStsNoErr;
      }
   }
}
