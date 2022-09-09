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
//     ippsDiv_BN()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: ippsDiv_BN
//
// Purpose: Divide BigNums.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pA  == NULL
//                            pB  == NULL
//                            pQ  == NULL
//                            pR  == NULL
//    ippStsContextMatchErr   !BN_VALID_ID(pA)
//                            !BN_VALID_ID(pB)
//                            !BN_VALID_ID(pQ)
//                            !BN_VALID_ID(pR)
//    ippStsDivByZeroErr      BN_SIZE(pB) == 1 && BN_NUMBER(pB)[0] == 0
//    ippStsOutOfRangeErr     pQ and/or pR can not hold result
//    ippStsNoErr             no errors
//
// Parameters:
//    pA    source BigNum
//    pB    source BigNum
//    pQ    quotient BigNum
//    pR    reminder BigNum
//
//    A = Q*B + R, 0 <= val(R) < val(B), sgn(A)==sgn(R)
//
*F*/
IPPFUN(IppStatus, ippsDiv_BN, (IppsBigNumState* pA, IppsBigNumState* pB, IppsBigNumState* pQ, IppsBigNumState* pR))
{
   IPP_BAD_PTR4_RET(pA, pB, pQ, pR);

   IPP_BADARG_RET(!BN_VALID_ID(pA), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pB), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pQ), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pR), ippStsContextMatchErr);

   IPP_BADARG_RET(BN_SIZE(pB)== 1 && BN_NUMBER(pB)[0]==0, ippStsDivByZeroErr);

   IPP_BADARG_RET(BN_ROOM(pR)<BN_SIZE(pB), ippStsOutOfRangeErr);
   IPP_BADARG_RET((int)BN_ROOM(pQ)<(int)(BN_SIZE(pA)-BN_SIZE(pB)), ippStsOutOfRangeErr);

   {
      BNU_CHUNK_T* pDataA = BN_BUFFER(pA);
      cpSize nsA = BN_SIZE(pA);
      BNU_CHUNK_T* pDataB = BN_NUMBER(pB);
      cpSize nsB = BN_SIZE(pB);
      BNU_CHUNK_T* pDataQ = BN_NUMBER(pQ);
      cpSize nsQ;
      BNU_CHUNK_T* pDataR = BN_NUMBER(pR);
      cpSize nsR;

      COPY_BNU(pDataA, BN_NUMBER(pA), nsA);
      nsR = cpDiv_BNU(pDataQ, &nsQ, pDataA, nsA, pDataB, nsB);
      COPY_BNU(pDataR, pDataA, nsR);

      BN_SIGN(pQ) = BN_SIGN(pA)==BN_SIGN(pB)? ippBigNumPOS : ippBigNumNEG;
      BN_SIZE(pQ) = nsQ;
      if(nsQ==1 && pDataQ[0]==0) BN_SIGN(pQ) = ippBigNumPOS;

      BN_SIGN(pR) = BN_SIGN(pA);
      BN_SIZE(pR) = nsR;
      if(nsR==1 && pDataR[0]==0) BN_SIGN(pR) = ippBigNumPOS;

      return ippStsNoErr;
   }
}
