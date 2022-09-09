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
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: ippsBigNumGetSize
//
// Purpose: Returns size of BigNum ctx (bytes).
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtxSize == NULL
//    ippStsLengthErr         length < 1
//                            length > BITS2WORD32_SIZE(BN_MAXBITSIZE)
//    ippStsNoErr             no errors
//
// Parameters:
//    length  max BN length (32-bits segments)
//    pSize   pointer BigNum ctx size
//
*F*/
IPPFUN(IppStatus, ippsBigNumGetSize, (int length, cpSize *pCtxSize))
{
   IPP_BAD_PTR1_RET(pCtxSize);
   IPP_BADARG_RET(length<1 || length>BITS2WORD32_SIZE(BN_MAXBITSIZE), ippStsLengthErr);

   {
      /* convert length to the number of BNU_CHUNK_T */
      cpSize len = INTERNAL_BNU_LENGTH(length);

      /* reserve one BNU_CHUNK_T more for cpDiv_BNU,
         mul, mont exp operations */
      len++;

      *pCtxSize = (Ipp32s)sizeof(IppsBigNumState)
                + len*(Ipp32s)sizeof(BNU_CHUNK_T)
                + len*(Ipp32s)sizeof(BNU_CHUNK_T)
                + BN_ALIGNMENT-1;

      return ippStsNoErr;
   }
}
