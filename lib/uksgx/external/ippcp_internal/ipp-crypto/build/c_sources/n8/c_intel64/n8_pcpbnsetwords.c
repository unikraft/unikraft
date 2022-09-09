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
//     ippsSet_BN()
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: ippsSet_BN
//
// Purpose: Set BigNum.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pBN == NULL
//                            pData == NULL
//    ippStsContextMatchErr   !BN_VALID_ID(pBN)
//    ippStsLengthErr         length < 1
//    ippStsOutOfRangeErr     length > BN_ROOM(pBN)
//    ippStsNoErr             no errors
//
// Parameters:
//    sgn      sign
//    length   data size (in Ipp32u chunks)
//    pData    source data pointer
//    pBN      BigNum ctx
//
*F*/
IPPFUN(IppStatus, ippsSet_BN, (IppsBigNumSGN sgn, int length, const Ipp32u* pData,
                               IppsBigNumState* pBN))
{
   IPP_BAD_PTR2_RET(pData, pBN);

   IPP_BADARG_RET(!BN_VALID_ID(pBN), ippStsContextMatchErr);

   IPP_BADARG_RET(length<1, ippStsLengthErr);

   /* compute real size */
   FIX_BNU32(pData, length);

   {
      cpSize len = INTERNAL_BNU_LENGTH(length);
      IPP_BADARG_RET(len > BN_ROOM(pBN), ippStsOutOfRangeErr);

      ZEXPAND_COPY_BNU((Ipp32u*)BN_NUMBER(pBN), BN_ROOM(pBN)*(int)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)), pData, length);

      BN_SIZE(pBN) = len;

      if(length==1 && pData[0] == 0)
         sgn = ippBigNumPOS;  /* consider zero value as positive */
      BN_SIGN(pBN) = sgn;

      return ippStsNoErr;
   }
}
